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

#include "render_pass_lighting.hpp"
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
	void render_pass_lighting::init(const vector2ui16& size)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "lighting_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "lighting_ubo"});
		}

		create_textures(size);

		_shader_lighting = engine_resources::get().get_shader_direct(engine_resource_ident::shader_lighting).get_hw();

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_lighting)
			{
				_shader_lighting = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_lighting::uninit()
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

	void render_pass_lighting::prepare(proxy_manager& pm, const view& camera_view, u8 frame_index)
	{
		ZoneScoped;

		const u8					ambient_exists = pm.get_ambient_exists();
		const render_proxy_ambient& ambient		   = pm.get_ambient();
		const vector3				ambient_color  = ambient_exists ? ambient.base_color : vector3(0.1f, 0.1f, 0.1f);
		const u8					skybox_exists  = pm.get_skybox_exists();
		const render_proxy_skybox&	skybox		   = pm.get_skybox();
		const vector4				sky_start	   = skybox_exists ? skybox.start_color : vector4(0.2f, 0.1f, 0.2f, 1.0f);
		const vector4				sky_mid		   = skybox_exists ? skybox.mid_color : vector4(0.1f, 0.1f, 0.2f, 1.0f);
		const vector4				sky_end		   = skybox_exists ? skybox.end_color : vector4(0.2f, 0.1f, 0.1f, 1.0f);
		const vector4				fog_color	   = skybox_exists ? skybox.fog_color : vector4(0.0f, 0.0f, 0.0f, 0.0f);
		const vector2				fog_start_end  = skybox_exists ? vector2(skybox.fog_start, skybox.fog_end) : vector2::zero;

		per_frame_data& pfd = _pfd[frame_index];

		const ubo ubo_data = {
			.inverse_view_proj			 = camera_view.view_proj_matrix.inverse(),
			.ambient_color_plights_count = vector4(ambient_color.x, ambient_color.y, ambient_color.z, static_cast<f32>(_points_count_this_frame)),
			.view_position_slights_count = vector4(camera_view.position.x, camera_view.position.y, camera_view.position.z, static_cast<f32>(_spots_count_this_frame)),
			.sky_start					 = sky_start,
			.sky_mid					 = sky_mid,
			.sky_end					 = sky_end,
			.fog_color_and_density		 = fog_color,
			.fog_start_end				 = fog_start_end,
			.dir_lights_count			 = _dirs_count_this_frame,
			.cascade_levels_gpu_index	 = camera_view.cascsades_gpu_index,
			.cascade_count				 = static_cast<u32>(camera_view.cascades.size()),
			.near_plane					 = camera_view.near_plane,
			.far_plane					 = camera_view.far_plane,
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
	}

	void render_pass_lighting::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	queue_gfx		 = backend->get_queue_gfx();
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gfx_id	render_target	 = pfd.render_target;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();
		const gfx_id	sh				 = _shader_lighting;

		// RP constants.
		static_vector<gpu_index, GBUFFER_COLOR_TEXTURES + 10> rp_constants;
		rp_constants.push_back(gpu_index_rp_ubo);
		rp_constants.push_back(p.gpu_index_entities);
		rp_constants.push_back(p.gpu_index_point_lights);
		rp_constants.push_back(p.gpu_index_spot_lights);
		rp_constants.push_back(p.gpu_index_dir_lights);
		rp_constants.push_back(p.gpu_index_shadow_data_buffer);
		rp_constants.push_back(p.gpu_index_float_buffer);
		for (u32 i = 0; i < GBUFFER_COLOR_TEXTURES; i++)
			rp_constants.push_back(p.gpu_index_gbuffer_textures[i]);
		rp_constants.push_back(p.gpu_index_depth_texture);
		rp_constants.push_back(p.gpu_index_ao_out);
		SFG_ASSERT(rp_constants.size() < constant_index_object_constant0 - constant_index_rp_constant0 + 1);

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
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

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "lighting_pass");

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

		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants.data(), .offset = constant_index_rp_constant0, .count = static_cast<u8>(rp_constants.size()), .param_index = rpi_constants});

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

	void render_pass_lighting::resize(const vector2ui16& size)
	{
		destroy_textures();
		create_textures(size);
	}

	void render_pass_lighting::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.render_target);
		}
	}

	void render_pass_lighting::create_textures(const vector2ui16& sz)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.render_target = backend->create_texture({
				.texture_format = render_target_definitions::get_format_lighting(),
				.size			= sz,
				.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 1.0f},
				.debug_name		= "lighting_rt",
			});

			pfd.gpu_index_render_target = backend->get_texture_gpu_index(pfd.render_target, 1);
		}
	}
}
