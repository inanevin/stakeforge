/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "editor/world/editor_world_rendering.hpp"
#include "editor/world/editor_world.hpp"
#include "editor/world/editor_world_render_context.hpp"
#include "editor/ui/panels/editor_theme.hpp"

#include <sfg/core/gfx/backend/backend.hpp>
#include <sfg/core/gfx/common/barrier_description.hpp>
#include <sfg/core/gfx/common/commands.hpp>
#include <sfg/core/gfx/util/gfx_util.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/math/math.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/runtime/render/render_view.hpp>
#include <sfg/core/runtime/render/render_resources.hpp>
#include <sfg/core/runtime/render/world_draw.hpp>
#include <sfg/core/runtime/render/world_render_snapshot.hpp>
#include <sfg/core/runtime/render/world_render_context.hpp>
#include <sfg/core/runtime/world_renderer/world_renderer.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define EDITOR_WORLD_GIZMO_SCALE_FULL				  100
#define EDITOR_WORLD_GIZMO_ROTATION_ORIENTATION_SCALE 55

	void editor_world_rendering_t::render_world(const editor_world_render_context_t& ctx,
												const world_renderer_t&				 world_renderer,
												const editor_world_snapshot_data_t&	 snapshot_data,
												world_render_prep_data_t&			 prep_data,
												f32									 interpolation_alpha,
												u8									 frame_index,
												gpu_index_t							 global_cbv_index,
												gfx_handle_t						 global_layout)
	{
		gfx_backend&	   backend		  = gfx_backend::get();
		const gfx_handle_t cmd			  = ctx.get_command_buffer(frame_index);
		const gfx_handle_t editor_texture = ctx.get_world_texture(frame_index);

		const world_render_context_t&  world_ctx = world_renderer.get_render_context();
		const world_render_snapshot_t& snapshot	 = world_renderer.get_render_snapshot();

		prep_icons(ctx, snapshot, snapshot_data, interpolation_alpha, frame_index);

		backend.reset_command_buffer(cmd);

		render_outlines(ctx, world_renderer, snapshot_data, prep_data, frame_index, global_cbv_index, global_layout);
		render_object_ids(ctx, world_renderer, snapshot_data, prep_data, frame_index, global_cbv_index, global_layout);
		blit_world_texture(ctx, world_ctx, world_renderer.get_depth_texture_index(frame_index), snapshot, snapshot_data, prep_data, interpolation_alpha, frame_index);

		const barrier_t end_barrier{
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture	 = editor_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
		backend.close_command_buffer(cmd);

		const gfx_handle_t queue_gfx = backend.get_queue_gfx();

		backend.submit_commands(queue_gfx, &cmd, 1);
	}

	void editor_world_rendering_t::render_outlines(
		const editor_world_render_context_t& ctx, const world_renderer_t& world_renderer, const editor_world_snapshot_data_t& snapshot_data, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				 = ctx.get_command_buffer(frame_index);
		const gfx_handle_t selection_texture = ctx.get_selection_texture(frame_index);
		const vec2u16_t	   size				 = ctx.get_size();

		const barrier_t begin_barrier = {
			.from_states = resource_state_ps_resource,
			.to_states	 = resource_state_render_target,
			.texture	 = selection_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});

		const render_pass_color_attachment_t selection_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 0.0f),
			.texture	 = selection_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_selection_mask");
		backend.cmd_begin_render_pass(cmd, {.color_attachments = &selection_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		world_renderer.draw_selection(cmd, prep_data, {.data = snapshot_data.selected_entities.data(), .size = snapshot_data.selected_entities.size()}, frame_index, global_cbv_index, global_layout);

		backend.cmd_end_render_pass(cmd, {});

		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t end_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_ps_resource,
			.texture	 = selection_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &end_barrier, .barrier_count = 1});
	}

	void editor_world_rendering_t::render_object_ids(
		const editor_world_render_context_t& ctx, const world_renderer_t& world_renderer, const editor_world_snapshot_data_t& snapshot_data, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		ZoneScoped;

		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd				 = ctx.get_command_buffer(frame_index);
		const gfx_handle_t object_id_texture = ctx.get_object_id_texture(frame_index);
		const gfx_handle_t depth_texture	 = world_renderer.get_depth_texture(frame_index);

		const vec2u16_t size = ctx.get_size();

		const barrier_t begin_barrier = {
			.from_states = resource_state_copy_source,
			.to_states	 = resource_state_render_target,
			.texture	 = object_id_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(static_cast<f32>(NULL_ENTITY_ID), 0.0f, 0.0f, 0.0f),
			.texture	 = object_id_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_object_ids");
		backend.cmd_begin_render_pass_depth_read_only(cmd,
													  {
														  .color_attachments = &color_attachment,
														  .depth_stencil_attachment =
															  {
																  .texture			= depth_texture,
																  .clear_stencil	= 0,
																  .clear_depth		= 0.0f,
																  .depth_load_op	= load_op::load,
																  .stencil_load_op	= load_op::none,
																  .depth_store_op	= store_op::store,
																  .stencil_store_op = store_op::none,
																  .view_index		= 1,
															  },
														  .color_attachment_count = 1,
													  });
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});

		// draw visible object ids
		world_renderer.draw_entity_ids(cmd, prep_data, frame_index, global_cbv_index, global_layout);
		draw_icons(ctx, ctx.get_icon_id_shader(), NULL_GPU_INDEX, static_cast<u32>(snapshot_data.icons.size()), frame_index);

		backend.cmd_end_render_pass(cmd, {});
		END_DEBUG_EVENT((&backend), cmd);

		const barrier_t readback_barrier = {
			.from_states = resource_state_render_target,
			.to_states	 = resource_state_copy_source,
			.texture	 = object_id_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &readback_barrier, .barrier_count = 1});
		backend.cmd_copy_texture_to_buffer(cmd,
										   {
											   .dest_buffer = ctx.get_object_id_readback(frame_index),
											   .src_texture = object_id_texture,
											   .size		= vec2u_t(size.x, size.y),
											   .bpp			= static_cast<u8>(sizeof(u32)),
										   });
	}

	void editor_world_rendering_t::prep_icons(const editor_world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const editor_world_snapshot_data_t& snapshot_data, f32 interpolation_alpha, u8 frame_index)
	{
		render_view_t view = {};

		view.calculate(snapshot.main_view, ctx.get_size(), interpolation_alpha);

		editor_world_icon_gpu_t* mapped_icons	  = reinterpret_cast<editor_world_icon_gpu_t*>(ctx.get_mapped_icon_buffer(frame_index));
		render_resources_t&		 render_resources = render_resources_t::get();

		for (u32 i = 0; i < snapshot_data.icons.size(); ++i)
		{
			const editor_world_icon_t& icon		= snapshot_data.icons[i];
			const vec3f_t			   position = vec3f_t::lerp(icon.previous_position, icon.position, interpolation_alpha);

			mapped_icons[i] = {
				.clip_position = view.view_proj * vec4f_t(position.x, position.y, position.z, 1.0f),
				.size_px	   = icon.size_px,
				.screen_offset = icon.screen_offset,
				.texture_index = render_resources.get_texture_gpu_index(icon.texture, 0),
				.entity_id	   = icon.entity_id,
			};
		}
	}

	void editor_world_rendering_t::draw_icons(const editor_world_render_context_t& ctx, gfx_handle_t shader, gpu_index_t depth_texture_index, u32 icon_count, u8 frame_index)
	{
		if (icon_count == 0)
			return;

		gfx_backend&	   backend				 = gfx_backend::get();
		const gfx_handle_t cmd					 = ctx.get_command_buffer(frame_index);
		const vec2u16_t	   size					 = ctx.get_size();
		const gpu_index_t  buffer_constants[2]	 = {ctx.get_icon_buffer_index(frame_index), depth_texture_index};
		const u32		   viewport_constants[2] = {size.x, size.y};

		backend.cmd_bind_constants(cmd, {.data = buffer_constants, .offset = constant_rp0, .count = 2, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = viewport_constants, .offset = constant_rp2, .count = 2, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = shader});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 6, .instance_count = icon_count, .start_vertex_location = 0, .start_instance_location = 0});
	}

	void editor_world_rendering_t::blit_world_texture(const editor_world_render_context_t& ctx,
													  const world_render_context_t&		   world_ctx,
													  gpu_index_t						   depth_texture_index,
													  const world_render_snapshot_t&	   snapshot,
													  const editor_world_snapshot_data_t&  snapshot_data,
													  world_render_prep_data_t&			   prep_data,
													  f32								   interpolation_alpha,
													  u8								   frame_index)
	{
		ZoneScoped;

		const vec2u16_t size = ctx.get_size();
		render_view_t	view = {};

		view.calculate(snapshot.main_view, size, interpolation_alpha);

		const f32			  grid_period  = snapshot_data.grid.scale * 10000.0f;
		const f32			  grid_phase_x = view.pos.x - math::floor(view.pos.x / grid_period) * grid_period;
		const f32			  grid_phase_z = view.pos.z - math::floor(view.pos.z / grid_period) * grid_period;
		const editor_theme_t& theme		   = editor_theme_t::get();

		const editor_world_composite_data_t composite_data = {
			.proj			  = view.proj,
			.inv_proj		  = view.inv_proj,
			.inv_view		  = view.inv_view,
			.camera_position  = vec4f_t(view.pos.x, view.pos.y, view.pos.z, 1.0f),
			.camera_grid	  = vec4f_t(grid_phase_x, view.pos.y, grid_phase_z, snapshot_data.grid.enabled ? 1.0f : 0.0f),
			.grid_params	  = vec4f_t(snapshot_data.grid.scale, view.near_plane, view.far_plane, grid_period),
			.grid_minor_color = vec4f_t(theme.color_text2.x, theme.color_text2.y, theme.color_text2.z, 0.16f),
			.grid_major_color = vec4f_t(theme.color_text1.x, theme.color_text1.y, theme.color_text1.z, 0.30f),
			.grid_x_color	  = vec4f_t(theme.color_accent0.x, theme.color_accent0.y, theme.color_accent0.z, 0.65f),
			.grid_z_color	  = vec4f_t(theme.color_accent1.x, theme.color_accent1.y, theme.color_accent1.z, 0.65f),
			.params			  = vec4f_t(static_cast<f32>(size.x), static_cast<f32>(size.y), 2.0f, snapshot_data.selected_entities.empty() ? 0.0f : 1.0f),
			.selection_color  = theme.color_accent2,
			.snap_grid_origin = vec4f_t(snapshot_data.snap_grid.origin.x - view.pos.x, snapshot_data.snap_grid.origin.y - view.pos.y, snapshot_data.snap_grid.origin.z - view.pos.z, snapshot_data.snap_grid.spacing),
			.snap_grid_right  = vec4f_t(snapshot_data.snap_grid.right.x, snapshot_data.snap_grid.right.y, snapshot_data.snap_grid.right.z, snapshot_data.snap_grid.radius),
			.snap_grid_up	  = vec4f_t(snapshot_data.snap_grid.up.x, snapshot_data.snap_grid.up.y, snapshot_data.snap_grid.up.z, 0.0f),
			.snap_grid_center = vec4f_t(snapshot_data.snap_grid.center.x, snapshot_data.snap_grid.center.y, 0.0f, 0.0f),
		};

		SFG_MEMCPY(ctx.get_mapped_composite_data(frame_index), &composite_data, sizeof(editor_world_composite_data_t));

		const bool render_gizmo = snapshot_data.gizmo.control_type != editor_transform_control_type_e::invalid;

		if (render_gizmo)
		{
			static const quat_t axis_rotations[3] = {
				quat_t::angle_axis(-90.0f, {0.0f, 0.0f, 1.0f}),
				quat_t::identity,
				quat_t::angle_axis(-90.0f, vec3f_t::right),
			};

			const vec3f_t position = vec3f_t::lerp(snapshot_data.gizmo.prev_position, snapshot_data.gizmo.position, interpolation_alpha);
			const quat_t  rotation = quat_t::slerp(snapshot_data.gizmo.prev_rotation, snapshot_data.gizmo.rotation, interpolation_alpha);

			editor_world_gizmo_gpu_data_t gizmo_data = {};

			for (u32 i = 0; i < 3; ++i)
				gizmo_data.models[i] = mat4x4_t::transform(position, rotation * axis_rotations[i], vec3f_t::one);

			gizmo_data.models[3] = mat4x4_t::transform(position, rotation, vec3f_t(editor_world_gizmo_t::CENTRAL_SIZE, editor_world_gizmo_t::CENTRAL_SIZE, editor_world_gizmo_t::CENTRAL_SIZE));
			gizmo_data.models[4] = mat4x4_t::transform(position, rotation, {editor_world_gizmo_t::PLANE_SIZE, editor_world_gizmo_t::PLANE_SIZE, editor_world_gizmo_t::PLANE_THICKNESS});
			gizmo_data.models[5] = mat4x4_t::transform(position, rotation, {editor_world_gizmo_t::PLANE_THICKNESS, editor_world_gizmo_t::PLANE_SIZE, editor_world_gizmo_t::PLANE_SIZE});
			gizmo_data.models[6] = mat4x4_t::transform(position, rotation, {editor_world_gizmo_t::PLANE_SIZE, editor_world_gizmo_t::PLANE_THICKNESS, editor_world_gizmo_t::PLANE_SIZE});

			const vec3f_t xy_offset = rotation * vec3f_t(editor_world_gizmo_t::PLANE_CENTER, editor_world_gizmo_t::PLANE_CENTER, 0.0f);
			const vec3f_t yz_offset = rotation * vec3f_t(0.0f, editor_world_gizmo_t::PLANE_CENTER, -editor_world_gizmo_t::PLANE_CENTER);
			const vec3f_t zx_offset = rotation * vec3f_t(editor_world_gizmo_t::PLANE_CENTER, 0.0f, -editor_world_gizmo_t::PLANE_CENTER);

			gizmo_data.offsets[4] = vec4f_t(xy_offset.x, xy_offset.y, xy_offset.z, 0.0f);
			gizmo_data.offsets[5] = vec4f_t(yz_offset.x, yz_offset.y, yz_offset.z, 0.0f);
			gizmo_data.offsets[6] = vec4f_t(zx_offset.x, zx_offset.y, zx_offset.z, 0.0f);

			gizmo_data.colors[0]   = theme.color_accent0;
			gizmo_data.colors[1]   = theme.color_accent_green;
			gizmo_data.colors[2]   = theme.color_accent1;
			gizmo_data.colors[3]   = theme.color_text0;
			gizmo_data.colors[4]   = (theme.color_accent0 + theme.color_accent_green) * 0.5f;
			gizmo_data.colors[5]   = (theme.color_accent_green + theme.color_accent1) * 0.5f;
			gizmo_data.colors[6]   = (theme.color_accent1 + theme.color_accent0) * 0.5f;
			gizmo_data.colors[4].w = 0.4f;
			gizmo_data.colors[5].w = 0.4f;
			gizmo_data.colors[6].w = 0.4f;

			const editor_gizmo_axis_e highlight_axis = snapshot_data.gizmo.active_axis != editor_gizmo_axis_e::invalid ? snapshot_data.gizmo.active_axis : snapshot_data.gizmo.hovered_axis;

			if (highlight_axis != editor_gizmo_axis_e::invalid)
				gizmo_data.colors[static_cast<u32>(highlight_axis)] = theme.color_accent2;

			gizmo_data.params = vec4f_t(editor_world_gizmo_t::PIXEL_SIZE, static_cast<f32>(size.y), math::tan(snapshot.main_view.fov_degrees * DEG_2_RAD * 0.5f), editor_world_gizmo_t::MIN_WORLD_SIZE);
			SFG_MEMCPY(ctx.get_mapped_gizmo_data(frame_index), &gizmo_data, sizeof(editor_world_gizmo_gpu_data_t));
		}

		gpu_index_t source_texture_index = NULL_GPU_INDEX;

		switch (snapshot_data.world_view)
		{
		case editor_world_view_e::gbuffer_albedo:
			source_texture_index = world_ctx.get_gbuffer_albedo_index(frame_index);
			break;
		case editor_world_view_e::gbuffer_orm:
			source_texture_index = world_ctx.get_gbuffer_orm_index(frame_index);
			break;
		case editor_world_view_e::gbuffer_normal:
			source_texture_index = world_ctx.get_gbuffer_normal_index(frame_index);
			break;
		case editor_world_view_e::gbuffer_emissive:
			source_texture_index = world_ctx.get_gbuffer_emissive_index(frame_index);
			break;
		case editor_world_view_e::lighting:
			source_texture_index = world_ctx.get_lighting_texture_index(frame_index);
			break;
		case editor_world_view_e::post_process:
		case editor_world_view_e::final:
		default:
			source_texture_index = world_ctx.get_world_texture_index(frame_index);
			break;
		}

		gfx_backend& backend = gfx_backend::get();

		const gfx_handle_t cmd					   = ctx.get_command_buffer(frame_index);
		const gfx_handle_t editor_texture		   = ctx.get_world_texture(frame_index);
		const gpu_index_t  selection_texture_index = ctx.get_selection_texture_index(frame_index);
		const gpu_index_t  composite_data_index	   = ctx.get_composite_data_index(frame_index);

		const barrier_t begin_barrier = {
			.from_states = resource_state_ps_resource,
			.to_states	 = resource_state_render_target,
			.texture	 = editor_texture,
			.flags		 = barrier_flags::baf_is_texture,
		};

		backend.cmd_barrier(cmd, {.barriers = &begin_barrier, .barrier_count = 1});

		const render_pass_color_attachment_t color_attachment = {
			.clear_color = vec4f_t(0.0f, 0.0f, 0.0f, 1.0f),
			.texture	 = editor_texture,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		BEGIN_DEBUG_EVENT((&backend), cmd, "editor_world_render_texture");

		backend.cmd_begin_render_pass(cmd, {.color_attachments = &color_attachment, .color_attachment_count = 1});
		backend.cmd_set_viewport(cmd, {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = size.x, .height = size.y});
		backend.cmd_set_scissors(cmd, {.x = 0, .y = 0, .width = size.x, .height = size.y});
		backend.cmd_bind_constants(cmd, {.data = &composite_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});

		const gpu_index_t obj_constants[3] = {source_texture_index, selection_texture_index, depth_texture_index};

		backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 3, .param_index = 0});
		backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_composite_shader()});
		backend.cmd_draw_instanced(cmd, {.vertex_count_per_instance = 3, .instance_count = 1, .start_vertex_location = 0, .start_instance_location = 0});

		if (!snapshot.debug_draw.triangle_indices.empty())
		{
			const gpu_index_t physics_rp_constant = world_ctx.get_view_render_pass_data_index(frame_index);

			backend.cmd_bind_constants(cmd, {.data = &physics_rp_constant, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_physics_debug_shader()});
			backend.cmd_bind_vertex_buffers(cmd, {.buffer = world_ctx.get_debug_triangle_vertex_buffer(frame_index), .slot = 0, .vertex_size = static_cast<u16>(sizeof(vertex_debug_triangle_t)), .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = world_ctx.get_debug_triangle_index_buffer(frame_index), .offset = 0, .index_size = static_cast<u8>(sizeof(primitive_index))});
			backend.cmd_draw_indexed_instanced(cmd,
											   {
												   .index_count_per_instance = static_cast<u32>(snapshot.debug_draw.triangle_indices.size()),
												   .instance_count			 = 1,
												   .start_index_location	 = 0,
												   .base_vertex_location	 = 0,
												   .start_instance_location	 = 0,
											   });
		}

		draw_icons(ctx, ctx.get_icon_shader(), depth_texture_index, static_cast<u32>(snapshot_data.icons.size()), frame_index);

		if (render_gizmo)
		{
			u8 mesh_index = 0;

			switch (snapshot_data.gizmo.control_type)
			{
			case editor_transform_control_type_e::move:
				mesh_index = 0;
				break;
			case editor_transform_control_type_e::rotate:
				mesh_index = 1;
				break;
			case editor_transform_control_type_e::scale:
				mesh_index = 2;
				break;
			default:
				SFG_ASSERT(false);
				break;
			}

			const editor_world_gizmo_mesh_t& mesh			  = ctx.get_gizmo_mesh(mesh_index);
			const render_resources_t&		 render_resources = render_resources_t::get();
			const gfx_handle_t				 vertex_buffer	  = render_resources.get_resource(mesh.vertex_buffer);
			const gfx_handle_t				 index_buffer	  = render_resources.get_resource(mesh.index_buffer);

			SFG_ASSERT(!vertex_buffer.is_null() && !index_buffer.is_null());

			const gpu_index_t view_data_index  = world_ctx.get_view_render_pass_data_index(frame_index);
			const gpu_index_t gizmo_data_index = ctx.get_gizmo_data_index(frame_index);

			backend.cmd_bind_constants(cmd, {.data = &view_data_index, .offset = constant_rp0, .count = 1, .param_index = 0});
			backend.cmd_bind_constants(cmd, {.data = &gizmo_data_index, .offset = constant_rp4, .count = 1, .param_index = 0});
			backend.cmd_bind_pipeline(cmd, {.pipeline = ctx.get_gizmo_shader()});

			if (snapshot_data.gizmo.control_type == editor_transform_control_type_e::rotate)
			{
				const editor_world_gizmo_mesh_t& orientation_mesh		   = ctx.get_gizmo_mesh(0);
				const gfx_handle_t				 orientation_vertex_buffer = render_resources.get_resource(orientation_mesh.vertex_buffer);
				const gfx_handle_t				 orientation_index_buffer  = render_resources.get_resource(orientation_mesh.index_buffer);

				SFG_ASSERT(!orientation_vertex_buffer.is_null() && !orientation_index_buffer.is_null());

				backend.cmd_bind_vertex_buffers(cmd, {.buffer = orientation_vertex_buffer, .slot = 0, .vertex_size = orientation_mesh.vertex_stride, .offset = 0});
				backend.cmd_bind_index_buffers(cmd, {.buffer = orientation_index_buffer, .offset = 0, .index_size = orientation_mesh.index_stride});

				for (u32 axis = 0; axis < 3; ++axis)
				{
					const u32 obj_constants[2] = {axis, EDITOR_WORLD_GIZMO_ROTATION_ORIENTATION_SCALE};

					backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
					backend.cmd_draw_indexed_instanced(cmd,
													   {
														   .index_count_per_instance = orientation_mesh.index_count,
														   .instance_count			 = 1,
														   .start_index_location	 = 0,
														   .base_vertex_location	 = 0,
														   .start_instance_location	 = 0,
													   });
				}
			}

			if (snapshot_data.gizmo.control_type == editor_transform_control_type_e::move)
			{
				const editor_world_gizmo_mesh_t& plane_mesh			 = ctx.get_gizmo_central_mesh(1);
				const gfx_handle_t				 plane_vertex_buffer = render_resources.get_resource(plane_mesh.vertex_buffer);
				const gfx_handle_t				 plane_index_buffer	 = render_resources.get_resource(plane_mesh.index_buffer);

				SFG_ASSERT(!plane_vertex_buffer.is_null() && !plane_index_buffer.is_null());

				backend.cmd_bind_vertex_buffers(cmd, {.buffer = plane_vertex_buffer, .slot = 0, .vertex_size = plane_mesh.vertex_stride, .offset = 0});
				backend.cmd_bind_index_buffers(cmd, {.buffer = plane_index_buffer, .offset = 0, .index_size = plane_mesh.index_stride});

				for (u32 plane = static_cast<u32>(editor_gizmo_axis_e::xy); plane <= static_cast<u32>(editor_gizmo_axis_e::zx); ++plane)
				{
					const u32 obj_constants[2] = {plane, EDITOR_WORLD_GIZMO_SCALE_FULL};

					backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
					backend.cmd_draw_indexed_instanced(cmd,
													   {
														   .index_count_per_instance = plane_mesh.index_count,
														   .instance_count			 = 1,
														   .start_index_location	 = 0,
														   .base_vertex_location	 = 0,
														   .start_instance_location	 = 0,
													   });
				}
			}

			backend.cmd_bind_vertex_buffers(cmd, {.buffer = vertex_buffer, .slot = 0, .vertex_size = mesh.vertex_stride, .offset = 0});
			backend.cmd_bind_index_buffers(cmd, {.buffer = index_buffer, .offset = 0, .index_size = mesh.index_stride});

			for (u32 axis = 0; axis < 3; ++axis)
			{
				const u32 obj_constants[2] = {axis, EDITOR_WORLD_GIZMO_SCALE_FULL};

				backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
				backend.cmd_draw_indexed_instanced(cmd,
												   {
													   .index_count_per_instance = mesh.index_count,
													   .instance_count			 = 1,
													   .start_index_location	 = 0,
													   .base_vertex_location	 = 0,
													   .start_instance_location	 = 0,
												   });
			}

			if (snapshot_data.gizmo.control_type == editor_transform_control_type_e::move || snapshot_data.gizmo.control_type == editor_transform_control_type_e::scale)
			{
				const u8						 central_mesh_index	   = snapshot_data.gizmo.control_type == editor_transform_control_type_e::move ? 0 : 1;
				const editor_world_gizmo_mesh_t& central_mesh		   = ctx.get_gizmo_central_mesh(central_mesh_index);
				const gfx_handle_t				 central_vertex_buffer = render_resources.get_resource(central_mesh.vertex_buffer);
				const gfx_handle_t				 central_index_buffer  = render_resources.get_resource(central_mesh.index_buffer);

				SFG_ASSERT(!central_vertex_buffer.is_null() && !central_index_buffer.is_null());

				backend.cmd_bind_vertex_buffers(cmd, {.buffer = central_vertex_buffer, .slot = 0, .vertex_size = central_mesh.vertex_stride, .offset = 0});
				backend.cmd_bind_index_buffers(cmd, {.buffer = central_index_buffer, .offset = 0, .index_size = central_mesh.index_stride});

				const u32 obj_constants[2] = {static_cast<u32>(editor_gizmo_axis_e::central), EDITOR_WORLD_GIZMO_SCALE_FULL};

				backend.cmd_bind_constants(cmd, {.data = obj_constants, .offset = constant_obj0, .count = 2, .param_index = 0});
				backend.cmd_draw_indexed_instanced(cmd,
												   {
													   .index_count_per_instance = central_mesh.index_count,
													   .instance_count			 = 1,
													   .start_index_location	 = 0,
													   .base_vertex_location	 = 0,
													   .start_instance_location	 = 0,
												   });
			}
		}

		backend.cmd_end_render_pass(cmd, {});

		END_DEBUG_EVENT((&backend), cmd);
	}
}
