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

#include "render_pass_debug_rendering.hpp"
#include "math/math.hpp"
#include "app/engine_resources.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/world/renderable.hpp"
#include "gfx/world/renderable_collector.hpp"

#include "world/world.hpp"
#include "physics/physics_world.hpp"
#include "physics/physics_debug_renderer.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_pass_debug::init(const vector2ui16& size, world& w)
	{
#ifdef JPH_DEBUG_RENDERER
		_phy_renderer = new physics_debug_renderer();
#endif
		_world = &w;
		_alloc.init(PASS_ALLOC_SIZE_DEBUG_RENDERING, 8);

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "phy_debug_cmd"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "phy_debug_ubo"});
			pfd.vtx_buffer_line.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_LINE * sizeof(vertex_3d_line),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_LINE * sizeof(vertex_3d_line),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.vtx_buffer_gui.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_GUI * sizeof(vertex_gui),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_GUI * sizeof(vertex_gui),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.vtx_buffer_tri.create(
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_TRI * sizeof(vertex_simple),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_vtx",
				},
				{
					.size		= world_debug_rendering::MAX_VERTEX_COUNT_TRI * sizeof(vertex_simple),
					.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_vtx",
				});

			pfd.idx_buffer_line.create(
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_LINE * sizeof(primitive_index),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_idx_line",
				},
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_LINE * sizeof(primitive_index),
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_idx_line",
				});

			pfd.idx_buffer_tri.create(
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_TRI * sizeof(primitive_index),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_idx_tri",
				},
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_TRI * sizeof(primitive_index),
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_idx_tri",
				});

			pfd.idx_buffer_gui.create(
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_GUI * sizeof(vekt::index),
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "debug_idx_gui",
				},
				{
					.size		= world_debug_rendering::MAX_INDEX_COUNT_GUI * sizeof(vekt::index),
					.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
					.debug_name = "debug_idx_gui",
				});

			pfd.draw_data_gui.create(
				{
					.size			 = world_debug_rendering::MAX_DRAW_CALLS_GUI * sizeof(world_debug_rendering::gui_draw_call_data),
					.structure_size	 = sizeof(world_debug_rendering::gui_draw_call_data),
					.structure_count = world_debug_rendering::MAX_DRAW_CALLS_GUI,
					.flags			 = resource_flags::rf_cpu_visible,
					.debug_name		 = "debug_dd_gui",
				},
				{
					.size			 = world_debug_rendering::MAX_DRAW_CALLS_GUI * sizeof(world_debug_rendering::gui_draw_call_data),
					.structure_size	 = sizeof(world_debug_rendering::gui_draw_call_data),
					.structure_count = world_debug_rendering::MAX_DRAW_CALLS_GUI,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only,
					.debug_name		 = "debug_dd_gui",
				});
		}

		_shader_debug_triangle	  = engine_resources::get().get_shader_direct(engine_resource_ident::shader_debug_triangle).get_hw();
		_shader_debug_line		  = engine_resources::get().get_shader_direct(engine_resource_ident::shader_debug_line).get_hw();
		_shader_debug_gui_default = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_default).get_hw(shader_variant_flags::variant_flag_gui_3d);
		_shader_debug_gui_text	  = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_text).get_hw(shader_variant_flags::variant_flag_gui_3d);
		_shader_debug_gui_sdf	  = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_sdf).get_hw(shader_variant_flags::variant_flag_gui_3d);

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_debug_triangle)
			{
				_shader_debug_triangle = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_debug_line)
			{
				_shader_debug_line = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_gui_default)
			{
				_shader_debug_gui_default = sh.get_hw(shader_variant_flags::variant_flag_gui_3d);
				return;
			}

			if (type == engine_resource_ident::shader_gui_text)
			{
				_shader_debug_gui_text = sh.get_hw(shader_variant_flags::variant_flag_gui_3d);
				return;
			}

			if (type == engine_resource_ident::shader_gui_sdf)
			{
				_shader_debug_gui_sdf = sh.get_hw(shader_variant_flags::variant_flag_gui_3d);
				return;
			}
		});
#endif
	}

	void render_pass_debug::uninit()
	{
		ZoneScoped;

		_alloc.uninit();

#ifdef JPH_DEBUG_RENDERER
		delete _phy_renderer;
		_phy_renderer = nullptr;
#endif

		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_buffer);
			pfd.ubo.destroy();
			pfd.vtx_buffer_line.destroy();
			pfd.vtx_buffer_tri.destroy();
			pfd.vtx_buffer_gui.destroy();
			pfd.idx_buffer_line.destroy();
			pfd.idx_buffer_tri.destroy();
			pfd.idx_buffer_gui.destroy();
			pfd.draw_data_gui.destroy();
		}
	}

	void render_pass_debug::tick()
	{
#ifdef JPH_DEBUG_RENDERER
		if (!_physics_off)
			_phy_renderer->draw(*_world);
#endif
	}

	void render_pass_debug::prepare(proxy_manager& pm, const view& main_camera_view, const vector2ui16& resolution, u8 frame_index)
	{
		ZoneScoped;

		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_DRAW_CALLS_DEBUG_RENDERING);

		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;
		backend->reset_command_buffer(cmd_buffer);

		const world_id mc				 = pm.get_main_camera();
		vector3		   main_camera_right = vector3::right;
		vector3		   main_camera_up	 = vector3::up;

		if (mc != NULL_WORLD_ID)
		{
			render_proxy_entity& e = pm.get_entity(pm.get_camera(mc).entity);
			main_camera_right	   = e.rotation.get_right();
			main_camera_up		   = e.rotation.get_up();
		}

		const ubo ubo_data = {
			.view					  = main_camera_view.view_matrix,
			.proj					  = main_camera_view.proj_matrix,
			.view_proj				  = main_camera_view.view_proj_matrix,
			.cam_right_and_pixel_size = vector4(main_camera_right.x, main_camera_right.y, main_camera_right.z, 0.025f),
			.cam_up					  = vector4(main_camera_up.x, main_camera_up.y, main_camera_up.z, 0.0f),
			.resolution_and_planes	  = vector4(resolution.x, resolution.y, main_camera_view.near_plane, main_camera_view.far_plane),
			.sdf_thickness			  = 0.485f,
			.sdf_softness			  = 0.022f,
		};
		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));

		world_debug_rendering&				   dbg_rnd = _world->get_debug_rendering();
		const world_debug_rendering::snapshot* snap	   = dbg_rnd.get_read_snapshot();

		if (snap == nullptr)
			return;

		const u32 vtx_count_gui	 = snap->vtx_count_gui;
		const u32 vtx_count_line = snap->vtx_count_line;
		const u32 vtx_count_tri	 = snap->vtx_count_tri;
		const u32 idx_count_line = snap->idx_count_line;
		const u32 idx_count_tri	 = snap->idx_count_tri;
		const u32 idx_count_gui	 = snap->idx_count_gui;
		const u32 dc_count_gui	 = snap->dc_count_gui;
		const u32 dd_count_gui	 = snap->draw_data_count_gui;

		SFG_ASSERT((vtx_count_gui == 0 && idx_count_gui == 0) || (vtx_count_gui != 0 && idx_count_gui != 0));
		SFG_ASSERT((vtx_count_line == 0 && idx_count_line == 0) || (vtx_count_line != 0 && idx_count_line != 0));
		SFG_ASSERT((vtx_count_tri == 0 && idx_count_tri == 0) || (vtx_count_tri != 0 && idx_count_tri != 0));

		static_vector<barrier, 8> barriers;

		// -----------------------------------------------------------------------------
		// before copy
		// -----------------------------------------------------------------------------

		if (vtx_count_gui != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.idx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_non_ps_resource,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.draw_data_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_gui.buffer_data(0, snap->vertices_gui, sizeof(vertex_gui) * vtx_count_gui);
			pfd.idx_buffer_gui.buffer_data(0, snap->indices_gui, sizeof(vekt::index) * idx_count_gui);
			pfd.draw_data_gui.buffer_data(0, snap->draw_data_gui, sizeof(world_debug_rendering::gui_draw_call_data) * dd_count_gui);

			for (u32 i = 0; i < dc_count_gui; i++)
			{
				world_debug_rendering::gui_draw_call& dc = snap->draw_calls_gui[i];

				gfx_id pipeline = _shader_debug_gui_default;

				if (dc.font_idx != NULL_GPU_INDEX)
					pipeline = dc.is_icon ? _shader_debug_gui_sdf : _shader_debug_gui_text;

				_draw_stream.add_command({
					.start_index			 = dc.start_index,
					.index_count			 = dc.index_count,
					.base_vertex			 = dc.base_vertex,
					.material_constant_index = dc.draw_data_idx,
					.texture_constant_index	 = pfd.draw_data_gui.get_index(),
					.font_index				 = dc.font_idx,
					.vb_hw					 = pfd.vtx_buffer_gui.get_gpu(),
					.ib_hw					 = pfd.idx_buffer_gui.get_gpu(),
					.pipeline_hw			 = pipeline,
					.vertex_size			 = sizeof(vertex_gui),
					.idx_size				 = sizeof(vekt::index),
				});
			}
		}

		if (vtx_count_line != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.idx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_line.buffer_data(0, snap->vertices_line, sizeof(vertex_3d_line) * vtx_count_line);
			pfd.idx_buffer_line.buffer_data(0, snap->indices_line, sizeof(primitive_index) * idx_count_line);

			_draw_stream.add_command({
				.start_index			 = 0,
				.index_count			 = idx_count_line,
				.base_vertex			 = 0,
				.material_constant_index = NULL_GPU_INDEX,
				.texture_constant_index	 = NULL_GPU_INDEX,
				.font_index				 = NULL_GPU_INDEX,
				.vb_hw					 = pfd.vtx_buffer_line.get_gpu(),
				.ib_hw					 = pfd.idx_buffer_line.get_gpu(),
				.pipeline_hw			 = _shader_debug_line,
				.vertex_size			 = sizeof(vertex_3d_line),
				.idx_size				 = sizeof(primitive_index),
			});
		}

		if (vtx_count_tri != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.vtx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.idx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_tri.buffer_data(0, snap->vertices_tri, sizeof(vertex_simple) * vtx_count_tri);
			pfd.idx_buffer_tri.buffer_data(0, snap->indices_tri, sizeof(primitive_index) * idx_count_tri);

			_draw_stream.add_command({
				.start_index			 = 0,
				.index_count			 = idx_count_tri,
				.base_vertex			 = 0,
				.material_constant_index = NULL_GPU_INDEX,
				.texture_constant_index	 = NULL_GPU_INDEX,
				.font_index				 = NULL_GPU_INDEX,
				.vb_hw					 = pfd.vtx_buffer_tri.get_gpu(),
				.ib_hw					 = pfd.idx_buffer_tri.get_gpu(),
				.pipeline_hw			 = _shader_debug_triangle,
				.vertex_size			 = sizeof(vertex_simple),
				.idx_size				 = sizeof(primitive_index),
			});
		}

		// -----------------------------------------------------------------------------
		// perform barr?er
		// -----------------------------------------------------------------------------

		if (!barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<u16>(barriers.size()),
								 });
		}

		// -----------------------------------------------------------------------------
		// copy and post barriers
		// -----------------------------------------------------------------------------

		barriers.resize(0);
		if (vtx_count_gui != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = pfd.idx_buffer_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = pfd.draw_data_gui.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_gui.copy_region(cmd_buffer, 0, sizeof(vertex_gui) * vtx_count_gui);
			pfd.idx_buffer_gui.copy_region(cmd_buffer, 0, sizeof(vekt::index) * idx_count_gui);
			pfd.draw_data_gui.copy_region(cmd_buffer, 0, sizeof(world_debug_rendering::gui_draw_call_data) * dd_count_gui);
		}

		if (vtx_count_line != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = pfd.idx_buffer_line.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_line.copy_region(cmd_buffer, 0, sizeof(vertex_3d_line) * vtx_count_line);
			pfd.idx_buffer_line.copy_region(cmd_buffer, 0, sizeof(primitive_index) * idx_count_line);
		}

		if (vtx_count_tri != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.vtx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = pfd.idx_buffer_tri.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			pfd.vtx_buffer_tri.copy_region(cmd_buffer, 0, sizeof(vertex_simple) * vtx_count_tri);
			pfd.idx_buffer_tri.copy_region(cmd_buffer, 0, sizeof(primitive_index) * idx_count_tri);
		}

		// -----------------------------------------------------------------------------
		// perform barr?er
		// -----------------------------------------------------------------------------

		if (!barriers.empty())
		{
			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<u16>(barriers.size()),
								 });
		}

		// -----------------------------------------------------------------------------
		// draw calls
		// -----------------------------------------------------------------------------

		_draw_stream.build();
	}

	void render_pass_debug::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend			 = gfx_backend::get();
		per_frame_data& pfd				 = _pfd[p.frame_index];
		const gfx_id	texture			 = p.input_texture;
		const gfx_id	depth			 = p.depth_texture;
		const gfx_id	cmd_buffer		 = pfd.cmd_buffer;
		const gpu_index gpu_index_rp_ubo = pfd.ubo.get_index();

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = texture,
			.load_op	 = load_op::load,
			.store_op	 = store_op::store,
		};

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "physics_debug_pass");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = &att,
														   .depth_stencil_attachment =
															   {
																   .texture		   = depth,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = 1,
													   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const u32 rp_constants[1] = {gpu_index_rp_ubo};
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants, .offset = constant_index_rp_constant0, .count = 1, .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(p.size.x), .height = static_cast<u16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<u16>(p.size.x),
									  .height	 = static_cast<u16>(p.size.y),

								  });

		_draw_stream.draw_no_clip(cmd_buffer);

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		static_vector<barrier, 2> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_depth_read | resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = p.depth_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_render_target,
			.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
			.resource	 = p.input_texture,
			.flags		 = barrier_flags::baf_is_texture,
		});

		backend->cmd_barrier(cmd_buffer,
							 {
								 .barriers		= barriers.data(),
								 .barrier_count = static_cast<u16>(barriers.size()),
							 });

		backend->close_command_buffer(cmd_buffer);
	}

}
