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

#include "render_pass_post_combiner.hpp"
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

#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_post_combiner::init(const vector2ui16& size)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "post_comb_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "post_combiner_ubo"});
		}

		create_textures(size);

		u32 variant_flags = 0;
#ifdef SFG_TOOLMODE
		variant_flags |= shader_variant_flags::variant_flag_selection_outline;
#endif
		_shader_post_combiner = engine_resources::get().get_shader_direct(engine_resource_ident::shader_post_combiner).get_hw(variant_flags);

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([&](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_post_combiner)
			{
				u32 vf = 0;
#ifdef SFG_TOOLMODE
				vf |= shader_variant_flags::variant_flag_selection_outline;
#endif

				_shader_post_combiner = sh.get_hw(vf);
				return;
			}
		});
#endif
	}

	void render_pass_post_combiner::uninit()
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

	void render_pass_post_combiner::prepare(proxy_manager& pm, u8 frame_index, const vector2ui16& resolution)
	{
		ZoneScoped;

		const u8						 post_exists		  = pm.get_post_process_exists();
		const u8						 bloom_exists		  = pm.get_bloom_exists();
		const render_proxy_post_process& post				  = pm.get_post_process();
		f32								 bloom_strength		  = post_exists ? post.bloom_strength : 0.04f;
		const f32						 exposure			  = post_exists ? post.exposure : 1.0f;
		const int						 tonemap_mode		  = post_exists ? post.tonemap_mode : 1;
		const f32						 saturation			  = post_exists ? post.saturation : 1.0f;
		const f32						 wb_temp			  = post_exists ? post.wb_temp : 0.0f;
		const f32						 wb_tint			  = post_exists ? post.wb_tint : 0.0f;
		const f32						 reinhard_white_point = post_exists ? post.reinhard_white_point : 6.0f;

		per_frame_data& pfd = _pfd[frame_index];

		const ubo ubo_data = {
			.screen_size		  = vector2(static_cast<f32>(resolution.x), static_cast<f32>(resolution.y)),
			.bloom_strength		  = bloom_strength,
			.exposure			  = exposure,
			.tonemap_mode		  = tonemap_mode,
			.saturation			  = saturation,
			.wb_temp			  = wb_temp,
			.wb_tint			  = wb_tint,
			.reinhard_white_point = reinhard_white_point,
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_post_combiner::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend						= gfx_backend::get();
		per_frame_data& pfd							= _pfd[p.frame_index];
		const gfx_id	queue_gfx					= backend->get_queue_gfx();
		const gfx_id	cmd_buffer					= pfd.cmd_buffer;
		const gfx_id	render_target				= pfd.render_target;
		const gpu_index gpu_index_ubo				= pfd.ubo.get_index();
		const gpu_index gpu_index_lighting			= p.gpu_index_lighting;
		const gpu_index gpu_index_bloom				= p.gpu_index_bloom;
		const gpu_index gpu_index_selection_outline = p.gpu_index_selection_outline;
		const gfx_id	sh							= _shader_post_combiner;

		// RP constants.

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_render_target,
			.resource	 = render_target,
			.flags		 = barrier_flags::baf_is_texture,
		});
		backend->reset_command_buffer(cmd_buffer);
		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });
		barriers.resize(0);

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "post_combiner_pass");

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = render_target,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = &att,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const u32 constants[4] = {gpu_index_ubo, gpu_index_lighting, gpu_index_bloom, gpu_index_selection_outline};
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 4, .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(p.size.x), .height = static_cast<u16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<u16>(p.size.x),
									  .height	 = static_cast<u16>(p.size.y),

								  });
		backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = sh});

		backend->cmd_draw_instanced(cmd_buffer,
									{
										.vertex_count_per_instance = 3,
										.instance_count			   = 1,
									});

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_post_combiner::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_post_combiner::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_texture(pfd.render_target);
		}
	}

	void render_pass_post_combiner::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.render_target			= backend->create_texture({
						  .texture_format = render_target_definitions::get_format_post_combine(),
						  .size			  = sz,
						  .flags		  = texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
						  .views		  = {{.type = view_type::render_target}, {.type = view_type::sampled}},
						  .clear_values	  = {0.0f, 0.0f, 0.0f, 1.0f},
						  .debug_name	  = "post_combiner_rt",
			  });
			pfd.gpu_index_render_target = backend->get_texture_gpu_index(pfd.render_target, 1);
		}
	}
}
