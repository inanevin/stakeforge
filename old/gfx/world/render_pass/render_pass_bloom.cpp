/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "render_pass_bloom.hpp"
#include "math/math.hpp"
#include "app/engine_resources.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/texture_queue.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

	void render_pass_bloom::init(const vector2ui16& size)
	{
		gfx_backend* backend = gfx_backend::get();

		// ofd
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::compute, .debug_name = "bloom_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "bloom_ubo"});
		}

		create_textures(size);
		_shader_bloom_downsample = engine_resources::get().get_shader_direct(engine_resource_ident::shader_bloom_downsample).get_hw();
		_shader_bloom_upsample	 = engine_resources::get().get_shader_direct(engine_resource_ident::shader_bloom_upsample).get_hw();

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_bloom_downsample)
			{
				_shader_bloom_downsample = sh.get_hw();
				return;
			}
			if (type == engine_resource_ident::shader_bloom_upsample)
			{
				_shader_bloom_upsample = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_bloom::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
		}

		destroy_textures();
	}

	void render_pass_bloom::prepare(proxy_manager& pm, u8 frame_index)
	{
		ZoneScoped;

		const u8				  bloom_exists	= pm.get_bloom_exists();
		const render_proxy_bloom& bloom			= pm.get_bloom();
		const f32				  filter_radius = bloom_exists ? bloom.filter_radius : 0.01f;
		per_frame_data&			  pfd			= _pfd[frame_index];
		const ubo				  ubo_data		= {
								 .filter_radius = filter_radius,
		 };
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_bloom::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	  backend				  = gfx_backend::get();
		per_frame_data&	  pfd					  = _pfd[p.frame_index];
		const gfx_id	  cmd_buffer			  = pfd.cmd_buffer;
		const gfx_id	  shader_bloom_downsample = _shader_bloom_downsample;
		const gfx_id	  shader_bloom_upsample	  = _shader_bloom_upsample;
		const gfx_id	  output				  = pfd.downsample_out;
		const vector2ui16 res					  = p.size;

		const gpu_index gpu_index_ubo = pfd.ubo.get_index();

		backend->reset_command_buffer(cmd_buffer);

		static_vector<barrier, 2> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = output,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		barriers.resize(0);
		barriers.push_back({
			.resource = output,
			.flags	  = barrier_flags::baf_is_texture | barrier_flags::baf_is_uav,
		});

		backend->cmd_bind_layout_compute(cmd_buffer, {.layout = p.global_layout_compute});
		backend->cmd_bind_group_compute(cmd_buffer, {.group = p.global_group});
		backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&gpu_index_ubo, .offset = constant_index_rp_constant0, .count = 1, .param_index = rpi_constants});

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_bloom_downsample});

		gpu_index downsample_input	= p.gpu_index_lighting;
		gpu_index downsample_output = pfd.gpu_index_downsample_uav[0];

		for (u32 i = 0; i < MIPS_DS; i++)
		{
			const u32 group_size_x = 8;
			const u32 group_size_y = 8;
			const u32 half_w	   = res.x * math::pow(0.5f, static_cast<f32>(i + 1));
			const u32 half_h	   = res.y * math::pow(0.5f, static_cast<f32>(i + 1));
			const u32 gsx		   = (group_size_x + half_w - 1) / group_size_x;
			const u32 gsy		   = (group_size_y + half_h - 1) / group_size_y;

			if (gsx == 0 || gsy == 0)
				continue;

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "bloom_downsample");

			{
				const u32 constants[5] = {half_w, half_h, downsample_input, downsample_output, i};
				backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant1, .count = 5, .param_index = rpi_constants});
			}

			backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
			END_DEBUG_EVENT(backend, cmd_buffer);

			if (i < MIPS_DS - 1)
			{
				downsample_input  = pfd.gpu_index_downsample_srv[i];
				downsample_output = pfd.gpu_index_downsample_uav[i + 1];
			}

			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_bloom_upsample});

		gpu_index upsample_input  = pfd.gpu_index_downsample_srv[MIPS_DS - 1];
		gpu_index upsample_output = pfd.gpu_index_upsample_uav[MIPS_DS - 1];

		for (i32 i = MIPS_DS - 1; i >= 0; --i)
		{
			const u32 group_size_x = 8;
			const u32 group_size_y = 8;
			const u32 half_w	   = res.x * math::pow(0.5f, static_cast<f32>(i));
			const u32 half_h	   = res.y * math::pow(0.5f, static_cast<f32>(i));
			const u32 gsx		   = (group_size_x + half_w - 1) / group_size_x;
			const u32 gsy		   = (group_size_y + half_h - 1) / group_size_y;

			if (gsx == 0 || gsy == 0)
				continue;

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "bloom_upsample");

			{
				const u32 constants[4] = {half_w, half_h, upsample_input, upsample_output};
				backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant1, .count = 4, .param_index = rpi_constants});
			}

			backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
			END_DEBUG_EVENT(backend, cmd_buffer);

			if (i != 0)
			{
				upsample_input	= pfd.gpu_index_upsample_srv[i];
				upsample_output = pfd.gpu_index_upsample_uav[i - 1];
			}

			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		}

		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = output,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		barriers.resize(0);

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_bloom::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_bloom::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.downsample_out);
			backend->destroy_texture(pfd.upsample_out);
		}
	}

	void render_pass_bloom::create_textures(const vector2ui16& sz)
	{

		gfx_backend* backend = gfx_backend::get();

		vector<view_desc> views;

		for (u32 i = 0; i < MIPS_DS; i++)
		{
			views.push_back({
				.type			= view_type::gpu_write,
				.base_mip_level = static_cast<u8>(i),
				.mip_count		= 1,
			});
		}

		for (u32 i = 0; i < MIPS_DS; i++)
		{
			views.push_back({
				.type			= view_type::sampled,
				.base_mip_level = static_cast<u8>(i),
				.mip_count		= 1,
			});
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.downsample_out = backend->create_texture({
				.texture_format = render_target_definitions::get_format_lighting(),
				.size			= sz / 2,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_gpu_write,
				.views			= views,
				.mip_levels		= MIPS_DS,
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "bloom_ao_out",
			});

			pfd.upsample_out = backend->create_texture({
				.texture_format = render_target_definitions::get_format_lighting(),
				.size			= sz,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_gpu_write,
				.views			= views,
				.mip_levels		= MIPS_DS,
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "bloom_ao_upsample_out",
			});

			for (u32 i = 0; i < MIPS_DS; i++)
			{
				pfd.gpu_index_downsample_uav[i] = backend->get_texture_gpu_index(pfd.downsample_out, i);
				pfd.gpu_index_downsample_srv[i] = backend->get_texture_gpu_index(pfd.downsample_out, MIPS_DS + i);
				pfd.gpu_index_upsample_uav[i]	= backend->get_texture_gpu_index(pfd.upsample_out, i);
				pfd.gpu_index_upsample_srv[i]	= backend->get_texture_gpu_index(pfd.upsample_out, MIPS_DS + i);
			}
		}
	}
}
