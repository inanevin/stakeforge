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

#include "render_pass_ssao.hpp"
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

// misc
#include <tracy/Tracy.hpp>
#include <random>
#include <cmath>
#include <cstdint>

namespace SFG
{
	namespace
	{
		void make_ao_noise(u8* data, int size = 8, uint32_t seed = 1337)
		{
			std::mt19937						rng(seed);
			std::uniform_real_distribution<f32> U(0.0f, 1.0f);

			for (int y = 0; y < size; ++y)
			{
				for (int x = 0; x < size; ++x)
				{
					f32		theta = U(rng) * 6.28318530718f;
					vector2 v	  = {std::cos(theta), std::sin(theta)};

					// map from [-1,1] -> [0,255]
					uint8_t r = (uint8_t)std::round((v.x * 0.5f + 0.5f) * 255.0f);
					uint8_t g = (uint8_t)std::round((v.y * 0.5f + 0.5f) * 255.0f);

					size_t i	= (y * size + x) * 2;
					data[i + 0] = r;
					data[i + 1] = g;
				}
			}
		}
	}

	void render_pass_ssao::init(const vector2ui16& size, texture_queue& tq)
	{
		gfx_backend* backend = gfx_backend::get();

		// Noise texture.
		const vector2ui16 noise_size = vector2ui16(8, 8);
		const u8		  noise_bpp	 = 2;
		u8*				  noise_data = new u8[noise_size.x * noise_size.y * noise_bpp];

		make_ao_noise(noise_data, noise_size.x);
		_noise_tex				= backend->create_texture({
						 .texture_format = format::r8g8_unorm,
						 .size			 = noise_size,
						 .flags			 = texture_flags::tf_is_2d | texture_flags::tf_sampled,
						 .views			 = {{.type = view_type::sampled}},
						 .clear_values	 = {0.0f, 0.0f, 0.0f, 1.0f},
						 .debug_name	 = "ssao_noise",
		 });
		_noise_tex_intermediate = backend->create_resource({.size = backend->align_texture_size(backend->get_texture_size(noise_size.x, noise_size.y, noise_bpp)), .flags = resource_flags::rf_cpu_visible, .debug_name = "ssao_noise_intermediate"});
		_gpu_index_noise		= backend->get_texture_gpu_index(_noise_tex, 0);

		static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
		buffers.push_back({.pixels = noise_data, .size = noise_size, .bpp = noise_bpp});
		tq.add_request(buffers, _noise_tex, _noise_tex_intermediate, false, resource_state::resource_state_non_ps_resource, false);

		// ofd
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::compute, .debug_name = "ssao_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "ssao_ubo"});
		}

		create_textures(size);

		_shader_hbao		  = engine_resources::get().get_shader_direct(engine_resource_ident::shader_hbao).get_hw();
		_shader_hbao_upsample = engine_resources::get().get_shader_direct(engine_resource_ident::shader_hbao_upsample).get_hw();

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_hbao)
			{
				_shader_hbao = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_hbao_upsample)
			{
				_shader_hbao_upsample = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_ssao::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
		}

		backend->destroy_texture(_noise_tex);
		backend->destroy_resource(_noise_tex_intermediate);
		destroy_textures();
	}

	void render_pass_ssao::prepare(proxy_manager& pm, const view& camera_view, const vector2ui16& resolution, u8 frame_index)
	{
		ZoneScoped;

		const u8				 ssao_exists		 = pm.get_ssao_exists();
		const render_proxy_ssao& ssao				 = pm.get_ssao();
		const f32				 radius_world		 = ssao_exists ? ssao.radius_world : 0.75f;
		const f32				 bias				 = ssao_exists ? ssao.bias : 0.04f;
		const f32				 intensity			 = ssao_exists ? ssao.intensity : 1.25f;
		const f32				 power				 = ssao_exists ? ssao.power : 1.25f;
		const u32				 num_dirs			 = ssao_exists ? ssao.num_dirs : 8;
		const u32				 num_steps			 = ssao_exists ? ssao.num_steps : 6;
		const f32				 random_rot_strength = ssao_exists ? ssao.random_rot_strength : 1.5f;

		per_frame_data& pfd = _pfd[frame_index];

		const ubo ubo_data = {

			.proj_matrix	 = camera_view.proj_matrix,
			.inv_proj_matrix = camera_view.inv_proj_matrix,
			.view_matrix	 = camera_view.view_matrix,

			.full_size = vector2ui(resolution.x, resolution.y),
			.half_size = vector2ui(resolution.x / 2, resolution.y / 2),
			.inv_full  = vector2(1.0f / static_cast<f32>(resolution.x), 1.0f / static_cast<f32>(resolution.y)),
			.inv_half  = vector2(1.0f / (static_cast<f32>(resolution.x) * 0.5f), 1.0f / (static_cast<f32>(resolution.y) * 0.5f)),

			.z_near = camera_view.near_plane,
			.z_far	= camera_view.far_plane,

			.radius_world		 = radius_world,
			.bias				 = bias,
			.intensity			 = intensity,
			.power				 = power,
			.num_dirs			 = num_dirs,
			.num_steps			 = num_steps,
			.random_rot_strength = random_rot_strength,

		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_ssao::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	  backend			   = gfx_backend::get();
		per_frame_data&	  pfd				   = _pfd[p.frame_index];
		const gfx_id	  cmd_buffer		   = pfd.cmd_buffer;
		const gfx_id	  shader_hbao		   = _shader_hbao;
		const gfx_id	  shader_hbao_upsample = _shader_hbao_upsample;
		const gfx_id	  ao_output			   = pfd.ao_out;
		const vector2ui16 res				   = p.size;

		const gpu_index gpu_index_noise				  = _gpu_index_noise;
		const gpu_index gpu_index_output			  = pfd.gpu_index_ao_uav;
		const gpu_index gpu_index_output_srv		  = pfd.gpu_index_ao_srv;
		const gpu_index gpu_index_output_upsample_uav = pfd.gpu_index_ao_upsample_uav;
		const gpu_index gpu_index_ubo				  = pfd.ubo.get_index();

		backend->reset_command_buffer(cmd_buffer);

		static_vector<barrier, 2> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = ao_output,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		backend->cmd_bind_layout_compute(cmd_buffer, {.layout = p.global_layout_compute});
		backend->cmd_bind_group_compute(cmd_buffer, {.group = p.global_group});

		{
			const u32 rp_constants[5] = {gpu_index_ubo, p.gpu_index_depth, p.gpu_index_normals, gpu_index_noise, gpu_index_output};
			backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_hbao});
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 5, .param_index = rpi_constants});

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "ssao_pass");
			const u32 group_size_x = 8;
			const u32 group_size_y = 8;
			const u32 half_w	   = res.x * 0.5f;
			const u32 half_h	   = res.y * 0.5f;
			const u32 gsx		   = (group_size_x + half_w - 1) / group_size_x;
			const u32 gsy		   = (group_size_y + half_h - 1) / group_size_y;
			backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
			END_DEBUG_EVENT(backend, cmd_buffer);
		}

		barriers.resize(0);
		barriers.push_back({
			.resource = ao_output,
			.flags	  = barrier_flags::baf_is_texture | barrier_flags::baf_is_uav,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		{

			const u32 rp_constants[2] = {gpu_index_output_srv, gpu_index_output_upsample_uav};
			backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = shader_hbao_upsample});
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant3, .count = 2, .param_index = rpi_constants});

			BEGIN_DEBUG_EVENT(backend, cmd_buffer, "ssao_upsample_pass");
			const u32 group_size_x = 8;
			const u32 group_size_y = 8;
			const u32 half_w	   = res.x;
			const u32 half_h	   = res.y;
			const u32 gsx		   = (group_size_x + half_w - 1) / group_size_x;
			const u32 gsy		   = (group_size_y + half_h - 1) / group_size_y;
			backend->cmd_dispatch(cmd_buffer, {.group_size_x = gsx, .group_size_y = gsy, .group_size_z = 1});
			END_DEBUG_EVENT(backend, cmd_buffer);
		}

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = ao_output,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_ssao::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_ssao::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.ao_out);
			backend->destroy_texture(pfd.ao_upsample_out);
		}
	}

	void render_pass_ssao::create_textures(const vector2ui16& sz)
	{

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.ao_out = backend->create_texture({
				.texture_format = render_target_definitions::get_format_ssao_ao_out(),
				.size			= sz / 2,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_gpu_write,
				.views			= {{.type = view_type::gpu_write}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "ssao_ao_out",
			});

			pfd.ao_upsample_out = backend->create_texture({
				.texture_format = render_target_definitions::get_format_ssao_ao_out(),
				.size			= sz,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_gpu_write,
				.views			= {{.type = view_type::gpu_write}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "ssao_ao_upsample_out",
			});

			pfd.gpu_index_ao_uav		  = backend->get_texture_gpu_index(pfd.ao_out, 0);
			pfd.gpu_index_ao_srv		  = backend->get_texture_gpu_index(pfd.ao_out, 1);
			pfd.gpu_index_ao_upsample_uav = backend->get_texture_gpu_index(pfd.ao_upsample_out, 0);
			pfd.gpu_index_ao_upsample_srv = backend->get_texture_gpu_index(pfd.ao_upsample_out, 1);
		}
	}
}
