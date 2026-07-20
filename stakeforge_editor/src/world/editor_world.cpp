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

#include "world/editor_world.hpp"
#include "world/editor_world_camera_fly.hpp"
#include "world/editor_world_camera_orbit.hpp"
#include "world/editor_world_rendering.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/char_util.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/physics_collision_mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_SNAPSHOT_SLOT_COUNT		   3
#define EDITOR_WORLD_SNAPSHOT_SLOT_MASK			   0x3
#define EDITOR_WORLD_SNAPSHOT_FRESH_FLAG		   0x80
#define EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT	   32
#define EDITOR_WORLD_DEBUG_LINE_VERTEX_RESERVE	   (8192 * 4)
#define EDITOR_WORLD_DEBUG_LINE_INDEX_RESERVE	   (8192 * 6)
#define EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_RESERVE 164000
#define EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_RESERVE  164000
#define EDITOR_WORLD_DEBUG_TEXT_COMMAND_RESERVE	   256
#define EDITOR_WORLD_DEBUG_TEXT_BYTE_RESERVE	   32768
#define EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX		   16384
#define EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX		   24576
#define EDITOR_WORLD_LIGHT_MAX					   1024

	void editor_world_t::init(const world_init_config_t& init_config, editor_world_handle_t handle)
	{
		world_init_config_t world_config = init_config;
		world_config.debug_draw			 = {
			.font					 = editor_theme_t::get().font_default,
			.line_vertex_reserve	 = EDITOR_WORLD_DEBUG_LINE_VERTEX_RESERVE,
			.line_index_reserve		 = EDITOR_WORLD_DEBUG_LINE_INDEX_RESERVE,
			.triangle_vertex_reserve = EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_RESERVE,
			.triangle_index_reserve	 = EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_RESERVE,
			.text_command_reserve	 = EDITOR_WORLD_DEBUG_TEXT_COMMAND_RESERVE,
			.text_byte_reserve		 = EDITOR_WORLD_DEBUG_TEXT_BYTE_RESERVE,
			.text_vertex_max		 = EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX,
			.text_index_max			 = EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX,
		};

		_world.init(world_config);
		_edit_context.init();
		_edit_context.set_world(handle);
		_gizmo.init();

		_producer_slot		  = 0;
		_consumer_slot		  = 1;
		_latest_snapshot_slot = UINT8_MAX;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
		_pick_result.store(0, std::memory_order_relaxed);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_next_pick_request_id		 = 0;
		_camera_type				 = editor_world_camera_type_e::fly;
		_play_mode					 = editor_play_mode_e::none;
		_play_snapshot.shrink(0);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;

		_render_resolution = init_config.render_resolution;
		_render_context.init({
			.size				 = init_config.render_resolution,
			.light_max			 = EDITOR_WORLD_LIGHT_MAX,
			.line_vertex_max	 = EDITOR_WORLD_DEBUG_LINE_VERTEX_RESERVE,
			.line_index_max		 = EDITOR_WORLD_DEBUG_LINE_INDEX_RESERVE,
			.triangle_vertex_max = EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_RESERVE,
			.triangle_index_max	 = EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_RESERVE,
			.text_vertex_max	 = EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX,
			.text_index_max		 = EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX,
			.shadow_view_max	 = ENGINE_SHADOW_VIEW_MAX,
		});

		_render_prep_data.reserve(1000);

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
		{
			_snapshot_slots[i].reserve(8000,
									   EDITOR_WORLD_LIGHT_MAX,
									   EDITOR_WORLD_DEBUG_LINE_VERTEX_RESERVE,
									   EDITOR_WORLD_DEBUG_LINE_INDEX_RESERVE,
									   EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_RESERVE,
									   EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_RESERVE,
									   EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX,
									   EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX);

			editor_world_snapshot_data_t* data = new editor_world_snapshot_data_t();
			data->selected_entities.reserve(256);
			_snapshot_slots[i].user_data = data;
		}
	}

	void editor_world_t::uninit()
	{
		if (_play_mode != editor_play_mode_e::none)
		{
			_world.end_play();
			_play_mode = editor_play_mode_e::none;
		}

		_play_snapshot.shrink(0);

		_gizmo.uninit(_world);
		uninstall_camera();

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
		{
			delete static_cast<editor_world_snapshot_data_t*>(_snapshot_slots[i].user_data);
			_snapshot_slots[i].user_data = nullptr;
		}

		_render_context.uninit();
		_edit_context.uninit();
		_world.unload_all_used_resources();
		_world.uninit();
		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_pick_result.store(0, std::memory_order_relaxed);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_next_pick_request_id		 = 0;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;

		_render_resolution	  = vec2u16_t::zero;
		_producer_slot		  = 0;
		_consumer_slot		  = 0;
		_latest_snapshot_slot = UINT8_MAX;
	}

	void editor_world_t::resize(vec2u16_t render_resolution)
	{
		cancel_gizmo_action();

		_render_resolution = render_resolution;
		_render_context.resize(render_resolution);
		_pick_result.store(0, std::memory_order_relaxed);
		_last_render_pick_request_id = 0;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;
	}

	void editor_world_t::install_camera(editor_world_camera_type_e type)
	{
		uninstall_camera();

		_camera_type = type;

		switch (type)
		{
		case editor_world_camera_type_e::orbit:
			_camera = new editor_world_camera_orbit_t();
			break;
		case editor_world_camera_type_e::fly:
		default:
			_camera = new editor_world_camera_fly_t();
			break;
		}

		_camera->init(_world);
	}

	void editor_world_t::uninstall_camera()
	{
		if (_camera == nullptr)
			return;

		cancel_gizmo_action();

		_camera->uninit(_world);
		delete _camera;
		_camera = nullptr;
	}

	void editor_world_t::serialize_camera(nlohmann::json& out_json) const
	{
		SFG_ASSERT(_camera != nullptr);
		_camera->serialize(_world, out_json);
	}

	void editor_world_t::deserialize_camera(const nlohmann::json& in_json)
	{
		SFG_ASSERT(_camera != nullptr);
		_camera->deserialize(_world, in_json);
	}

	void editor_world_t::pass_camera_input(const editor_world_camera_input_t& input)
	{
		if (_camera != nullptr)
			_camera->pass_input(input);
	}

	void editor_world_t::reset_camera_input()
	{
		pass_camera_input({.reset = true});
	}

	void editor_world_t::tick_camera(f32 dt_seconds)
	{
		if (_camera != nullptr)
			_camera->tick(_world, dt_seconds);
	}

	void editor_world_t::fit_camera_to_bounds(const aabb_t& bounds)
	{
		if (_camera != nullptr)
			_camera->fit_to_bounds(_world, bounds);
	}

	void editor_world_t::update_gizmo_hover(vec2f_t relative_position)
	{
		if (_play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return;

		const entity_id_t camera_entity = _camera != nullptr ? _camera->get_entity() : NULL_ENTITY_ID;
		_gizmo.update_hover(_world, _edit_context, camera_entity, _render_resolution, relative_position);
	}

	void editor_world_t::clear_gizmo_hover()
	{
		_gizmo.clear_hover();
	}

	bool editor_world_t::begin_gizmo_action(vec2f_t relative_position)
	{
		if (_play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return false;

		const entity_id_t camera_entity = _camera != nullptr ? _camera->get_entity() : NULL_ENTITY_ID;
		return _gizmo.begin_action(_world, _edit_context, camera_entity, _render_resolution, relative_position);
	}

	void editor_world_t::update_gizmo_action(vec2f_t relative_position)
	{
		if (_play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return;

		const entity_id_t camera_entity = _camera != nullptr ? _camera->get_entity() : NULL_ENTITY_ID;
		_gizmo.update_action(_world, _edit_context, camera_entity, _render_resolution, relative_position);
	}

	void editor_world_t::end_gizmo_action()
	{
		if (_play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return;

		_gizmo.end_action(_world, _edit_context);
	}

	void editor_world_t::cancel_gizmo_action()
	{
		_gizmo.cancel_action(_world);
	}

	void editor_world_t::request_entity_pick(vec2f_t relative_position, bool incremental_selection)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		++_next_pick_request_id;

		if (_next_pick_request_id == 0)
			++_next_pick_request_id;

		_pending_pick_request = {
			.relative_position	   = relative_position,
			.id					   = _next_pick_request_id,
			.incremental_selection = incremental_selection,
		};
	}

	void editor_world_t::consume_entity_pick_result()
	{
		const u64 packed_result = _pick_result.exchange(0, std::memory_order_acquire);

		if (packed_result == 0)
			return;

		const u32 request_id = static_cast<u32>(packed_result >> EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT);

		if (request_id != _pending_pick_request.id)
			return;

		entity_id_t entity = static_cast<entity_id_t>(packed_result);

		if (entity != NULL_ENTITY_ID && !_world.is_alive(entity))
			entity = NULL_ENTITY_ID;

		const bool incremental_selection = _pending_pick_request.incremental_selection;
		_pending_pick_request			 = {};

		if (entity == NULL_ENTITY_ID)
		{
			_edit_context.clear_entity_selection();
			return;
		}

		if (!incremental_selection)
		{
			_edit_context.issue_entity_selection({.data = &entity, .size = 1}, entity);
			return;
		}

		const span_t<const entity_id_t> selected  = _edit_context.get_selected_entities();
		frame_vector_t<entity_id_t>		selection = {};
		selection.reserve(selected.size + 1);

		for (size_t i = 0; i < selected.size; ++i)
			selection.push_back(selected.data[i]);

		auto it = std::find(selection.begin(), selection.end(), entity);

		if (it == selection.end())
			selection.push_back(entity);
		else
			selection.erase(it);

		_edit_context.issue_entity_selection({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : entity);
	}

	void editor_world_t::save_play_snapshot()
	{
		SFG_ASSERT(_camera != nullptr);

		nlohmann::json snapshot = nlohmann::json::object();
		world_cooker_t::world_to_json(_world, snapshot);
		serialize_camera(snapshot["editor_camera"]);

		const string_t snapshot_text = snapshot.dump();

		_play_snapshot.shrink(0);
		_play_snapshot << snapshot_text;
	}

	void editor_world_t::restore_play_snapshot(bool keep_current_camera)
	{
		SFG_ASSERT(_play_snapshot.get_size() != 0);

		istream_t snapshot_stream(_play_snapshot.get_raw(), _play_snapshot.get_size());
		string_t  snapshot_text = {};

		snapshot_stream >> snapshot_text;

		const nlohmann::json snapshot = nlohmann::json::parse(snapshot_text, nullptr, false);
		SFG_ASSERT(!snapshot.is_discarded());
		nlohmann::json camera = snapshot.value<nlohmann::json>("editor_camera", nlohmann::json::object());

		if (keep_current_camera)
			serialize_camera(camera);

		// reset
		uninstall_camera();
		_world.clear_entities();
		world_cooker_t::world_from_json(_world, snapshot);

		// cam
		install_camera(_camera_type);
		deserialize_camera(camera);

		// reset
		_world.update_world_transforms(false);
		_play_snapshot.shrink(0);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_pick_result.store(0, std::memory_order_relaxed);
	}

	void editor_world_t::update_play_mode(editor_play_mode_e mode)
	{
		if (_play_mode == mode)
			return;

		if (_play_mode == editor_play_mode_e::none)
		{
			SFG_ASSERT(mode == editor_play_mode_e::play || mode == editor_play_mode_e::play_physics);

			if (mode == editor_play_mode_e::play)
			{
				cancel_gizmo_action();
				clear_gizmo_hover();
			}

			save_play_snapshot();

			if (mode == editor_play_mode_e::play)
				uninstall_camera();

			_world.begin_play();
			_play_mode = mode;
			return;
		}

		if (mode == editor_play_mode_e::none)
		{
			const bool keep_current_camera = _play_mode == editor_play_mode_e::play_physics || _play_mode == editor_play_mode_e::play_physics_paused;
			cancel_gizmo_action();
			clear_gizmo_hover();
			_world.end_play();
			_play_mode = editor_play_mode_e::none;
			restore_play_snapshot(keep_current_camera);
			return;
		}

		const bool full_pause_transition	= (_play_mode == editor_play_mode_e::play && mode == editor_play_mode_e::play_paused) || (_play_mode == editor_play_mode_e::play_paused && mode == editor_play_mode_e::play);
		const bool physics_pause_transition = (_play_mode == editor_play_mode_e::play_physics && mode == editor_play_mode_e::play_physics_paused) || (_play_mode == editor_play_mode_e::play_physics_paused && mode == editor_play_mode_e::play_physics);
		SFG_ASSERT(full_pause_transition || physics_pause_transition);
		_play_mode = mode;
	}

	void editor_world_t::begin_frame()
	{
		consume_entity_pick_result();
		_world.get_debug_draw().begin_frame();
	}

	void editor_world_t::draw_debug()
	{
		if (_edit_context.is_physics_debug_enabled() && _world.get_physics().is_init())
			_world.get_physics().draw_debug(_world.get_debug_draw());

		if (_gizmo.is_action_active())
		{
			const vec4f_t& color = editor_theme_t::get().color_accent2;
			_gizmo.draw_rotation_visualization(_world.get_debug_draw(), {color.x, color.y, color.z, color.w}, color_t::white, editor_theme_t::get().text_small_px_size * 1.5f);
		}

		const span_t<const entity_id_t> selected = _edit_context.get_selected_entities();

		if (selected.size != 0)
		{
			world_debug_draw_t&			 debug_draw		 = _world.get_debug_draw();
			const editor_theme_t&		 theme			 = editor_theme_t::get();
			const vec4f_t&				 accent_warn	 = theme.color_accent_warn;
			const vec4f_t&				 accent1		 = theme.color_accent1;
			const color_t				 debug_color	 = {accent_warn.x, accent_warn.y, accent_warn.z, accent_warn.w};
			const color_t				 collider_color	 = {accent1.x, accent1.y, accent1.z, accent1.w};
			const ecs_component_table_t& transform_table = _world.get_component_table(type_id_t<component_system_transform_t>::value);
			const ecs_component_table_t& camera_table	 = _world.get_component_table(type_id_t<component_camera_t>::value);
			const ecs_component_table_t& light_table	 = _world.get_component_table(type_id_t<component_light_t>::value);
			const ecs_component_table_t& collider_table	 = _world.get_component_table(type_id_t<component_collider_t>::value);

			for (size_t i = 0; i < selected.size; ++i)
			{
				const entity_id_t					entity	  = selected.data[i];
				const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);
				const component_camera_t*			camera	  = ecs_helpers_t::table_find_as_const<component_camera_t>(camera_table, entity);

				if (camera != nullptr)
				{
					SFG_ASSERT(_render_resolution.x != 0 && _render_resolution.y != 0);
					debug_draw.draw_frustum(transform.abs_pos,
											transform.abs_rot.get_forward(),
											transform.abs_rot.get_up(),
											camera->fov_degrees,
											static_cast<f32>(_render_resolution.x) / static_cast<f32>(_render_resolution.y),
											camera->near_plane,
											camera->far_plane,
											debug_color,
											2.0f,
											debug_draw_depth_e::always_visible);
				}

				const component_collider_t* collider = ecs_helpers_t::table_find_as_const<component_collider_t>(collider_table, entity);

				if (collider != nullptr)
				{
					const vec3f_t abs_scale		= vec3f_t::abs(transform.abs_scale);
					const quat_t  body_rotation = transform.abs_rot * collider->local_rotation;
					const vec3f_t body_position = transform.abs_pos + transform.abs_rot * (collider->local_position * transform.abs_scale);

					switch (collider->shape)
					{
					case physics_shape_type_e::box: {
						const mat4x3_t collider_transform = mat4x3_t::transform(body_position, body_rotation, vec3f_t::one);
						debug_draw.draw_box(collider_transform, vec3f_t::max(collider->half_extent * abs_scale, {0.001f, 0.001f, 0.001f}), collider_color, 2.0f, debug_draw_depth_e::always_visible);
						break;
					}
					case physics_shape_type_e::sphere: {
						const f32 radius = math::max(collider->radius * math::max(abs_scale.x, math::max(abs_scale.y, abs_scale.z)), 0.001f);
						debug_draw.draw_sphere(body_position, radius, collider_color, 2.0f, debug_draw_depth_e::always_visible);
						break;
					}
					case physics_shape_type_e::capsule: {
						const f32 radius	  = math::max(collider->radius * math::max(abs_scale.x, abs_scale.z), 0.001f);
						const f32 half_height = math::max(collider->half_height * abs_scale.y, 0.001f);
						debug_draw.draw_capsule(body_position, radius, half_height, body_rotation.get_up(), collider_color, 2.0f, debug_draw_depth_e::always_visible);
						break;
					}
					case physics_shape_type_e::cylinder: {
						const f32 radius	  = math::max(collider->radius * math::max(abs_scale.x, abs_scale.z), 0.001f);
						const f32 half_height = math::max(collider->half_height * abs_scale.y, 0.001f);
						debug_draw.draw_cylinder(body_position, radius, half_height, body_rotation.get_up(), collider_color, 2.0f, debug_draw_depth_e::always_visible);
						break;
					}
					case physics_shape_type_e::mesh: {
						const physics_collision_mesh_runtime_t* collision_mesh = resource_manager_t::get().find_runtime<physics_collision_mesh_runtime_t>(collider->collision_mesh);

						if (collision_mesh == nullptr)
							break;

						const chunk_allocator_t& memory	  = resource_manager_t::get().get_memory();
						const vec3f_t*			 vertices = memory.get<vec3f_t>(collision_mesh->vertices);
						const primitive_index*	 indices  = memory.get<primitive_index>(collision_mesh->indices);

						for (u32 i = 0; i < collision_mesh->index_count; i += 3)
						{
							const vec3f_t p0 = body_position + body_rotation * (vertices[indices[i]] * transform.abs_scale);
							const vec3f_t p1 = body_position + body_rotation * (vertices[indices[i + 1]] * transform.abs_scale);
							const vec3f_t p2 = body_position + body_rotation * (vertices[indices[i + 2]] * transform.abs_scale);
							debug_draw.draw_triangle(p0, p1, p2, collider_color);
						}

						break;
					}
					}
				}

				const component_light_t* light = ecs_helpers_t::table_find_as_const<component_light_t>(light_table, entity);

				if (light == nullptr)
					continue;

				const vec3f_t forward = transform.abs_rot.get_forward();

				switch (light->type)
				{
				case light_type_e::directional: {
					const vec3f_t offset = transform.abs_rot.get_right() * 0.25f;
					debug_draw.draw_line(transform.abs_pos - offset, transform.abs_pos - offset + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					debug_draw.draw_line(transform.abs_pos, transform.abs_pos + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					debug_draw.draw_line(transform.abs_pos + offset, transform.abs_pos + offset + forward * 2.0f, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				}
				case light_type_e::point:
					if (light->range > 0.0f)
						debug_draw.draw_sphere(transform.abs_pos, light->range, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				case light_type_e::spot:
					if (light->range > 0.0f && light->outer_cone_degrees > 0.0f)
						debug_draw.draw_frustum(transform.abs_pos, forward, transform.abs_rot.get_up(), light->outer_cone_degrees * 2.0f, 1.0f, 0.0f, light->range, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				case light_type_e::area:
					if (light->area_size.x > 0.0f && light->area_size.y > 0.0f)
						debug_draw.draw_rectangle(transform.abs_pos, transform.abs_rot.get_right(), transform.abs_rot.get_up(), light->area_size, debug_color, 2.0f, debug_draw_depth_e::always_visible);
					break;
				}
			}
		}

		if (_edit_context.is_bounding_boxes_enabled() && _latest_snapshot_slot != UINT8_MAX)
		{
			const world_render_snapshot_t& snapshot		= _snapshot_slots[_latest_snapshot_slot];
			world_debug_draw_t&			   debug_draw	= _world.get_debug_draw();
			const editor_theme_t&		   theme		= editor_theme_t::get();
			const color_t				   bounds_color = {theme.color_highlight.x, theme.color_highlight.y, theme.color_highlight.z, theme.color_highlight.w};

			for (const world_draw_t& draw : snapshot.draws)
			{
				const world_render_entity_t& entity		   = snapshot.entities[draw.entity_index];
				const vec3f_t				 center		   = (draw.aabb.bounds_min + draw.aabb.bounds_max) * 0.5f;
				const vec3f_t				 dimensions	   = draw.aabb.bounds_half_extent * 2.0f;
				const mat4x3_t				 box_transform = entity.transform * mat4x3_t::translation(center);
				const vec3f_t				 text_position = entity.transform * vec3f_t(center.x, draw.aabb.bounds_max.y, center.z);

				debug_draw.draw_box(box_transform, draw.aabb.bounds_half_extent, bounds_color, 2.0f, debug_draw_depth_e::always_visible);

				char  dimensions_text[96] = {};
				char* text_cur			  = dimensions_text;
				char* text_end			  = dimensions_text + sizeof(dimensions_text);
				char_util::append_double(text_cur, text_end, dimensions.x, 2);
				char_util::append(text_cur, text_end, " x ");
				char_util::append_double(text_cur, text_end, dimensions.y, 2);
				char_util::append(text_cur, text_end, " x ");
				char_util::append_double(text_cur, text_end, dimensions.z, 2);
				debug_draw.draw_text_3d(text_position, dimensions_text, bounds_color, theme.text_small_px_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
			}
		}
	}

	void editor_world_t::update_world_transforms(bool advance_interpolation)
	{
		_world.update_world_transforms(advance_interpolation);
	}

	void editor_world_t::produce_snapshot()
	{
		world_render_snapshot_t& snapshot = _snapshot_slots[_producer_slot];
		world_snapshot_producer_t::produce(_world, snapshot, engine_runtime_t::get().get_project_settings());

		editor_world_snapshot_data_t&	data			= *static_cast<editor_world_snapshot_data_t*>(snapshot.user_data);
		const span_t<const entity_id_t> selected		= _edit_context.get_selected_entities();
		const ecs_component_table_t&	hierarchy_table = _world.get_component_table(type_id_t<component_hierarchy_t>::value);

		data.pick_request = _pending_pick_request;
		data.grid		  = {
			.scale	 = _edit_context.get_world_view_settings().grid_scale,
			.enabled = _edit_context.is_grid_enabled(),
		};
		data.gizmo = {};

		const bool		  gizmo_enabled = _play_mode != editor_play_mode_e::play && _play_mode != editor_play_mode_e::play_paused;
		const entity_id_t anchor		= gizmo_enabled ? _edit_context.get_mutable_entity_anchor(_world) : NULL_ENTITY_ID;

		if (anchor != NULL_ENTITY_ID)
		{
			const ecs_component_table_t&		transform_table = _world.get_component_table(type_id_t<component_system_transform_t>::value);
			const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, anchor);
			const bool							local			= _edit_context.get_transform_locality() == editor_transform_locality_e::local;
			data.gizmo											= {
				.prev_rotation = local ? transform.prev_abs_rot : quat_t::identity,
				.rotation	   = local ? transform.abs_rot : quat_t::identity,
				.prev_position = transform.prev_abs_pos,
				.position	   = transform.abs_pos,
				.control_type  = _edit_context.get_transform_control_type(),
			};
		}

		if (gizmo_enabled)
		{
			data.gizmo.hovered_axis = _gizmo.get_hovered_axis();
			data.gizmo.active_axis	= _gizmo.get_active_axis();
		}

		data.selected_entities.resize(0);
		data.selected_entities.reserve(selected.size);

		for (size_t i = 0; i < selected.size; ++i)
			data.selected_entities.push_back(selected.data[i]);

		for (size_t i = 0; i < data.selected_entities.size(); ++i)
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, data.selected_entities[i]);
			entity_id_t					 child	   = hierarchy.first_child;

			while (child != NULL_ENTITY_ID)
			{
				if (std::find(data.selected_entities.begin(), data.selected_entities.end(), child) == data.selected_entities.end())
					data.selected_entities.push_back(child);

				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
				child										 = child_hierarchy.next_sibling;
			}
		}

		_latest_snapshot_slot = _producer_slot;
		publish_snapshot();
	}

	void editor_world_t::publish_snapshot()
	{
		const u8 prev  = _snapshot_mailbox.exchange(_producer_slot | EDITOR_WORLD_SNAPSHOT_FRESH_FLAG, std::memory_order_release);
		_producer_slot = static_cast<u8>((prev & EDITOR_WORLD_SNAPSHOT_SLOT_MASK) % EDITOR_WORLD_SNAPSHOT_SLOT_COUNT);
	}

	const world_render_snapshot_t& editor_world_t::acquire_render_snapshot()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 cur = _snapshot_mailbox.load(std::memory_order_acquire);

		while (cur & EDITOR_WORLD_SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(cur, _consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				_consumer_slot = static_cast<u8>((cur & EDITOR_WORLD_SNAPSHOT_SLOT_MASK) % EDITOR_WORLD_SNAPSHOT_SLOT_COUNT);
				break;
			}
		}

		return _snapshot_slots[_consumer_slot];
	}

	void editor_world_t::render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		_render_prep_data.reset();

		const editor_world_snapshot_data_t& data = *static_cast<const editor_world_snapshot_data_t*>(snapshot.user_data);

		if (_object_id_readback_valid[frame_index] && data.pick_request.id != 0 && data.pick_request.id != _last_render_pick_request_id)
		{
			const vec2u16_t size  = _render_context.get_size();
			const vec2u16_t pixel = {
				data.pick_request.relative_position.x >= 1.0f ? static_cast<u16>(size.x - 1) : static_cast<u16>(data.pick_request.relative_position.x * size.x),
				data.pick_request.relative_position.y >= 1.0f ? static_cast<u16>(size.y - 1) : static_cast<u16>(data.pick_request.relative_position.y * size.y),
			};
			const entity_id_t entity		= _render_context.get_object_id(frame_index, pixel);
			const u64		  packed_result = static_cast<u64>(data.pick_request.id) << EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT | entity;
			_pick_result.store(packed_result, std::memory_order_release);
			_last_render_pick_request_id = data.pick_request.id;
		}

		world_rendering_t::render_world(_render_context.get_world_render_context(), snapshot, _render_prep_data, interpolation_alpha, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_outlines(_render_context, snapshot, _render_prep_data, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_object_ids(_render_context, snapshot, _render_prep_data, frame_index);
		editor_world_rendering_t::blit_world_texture(_render_context, snapshot, _render_prep_data, interpolation_alpha, frame_index);
		_object_id_readback_valid[frame_index] = true;
	}
}
