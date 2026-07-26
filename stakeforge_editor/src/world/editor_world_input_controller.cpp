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

#include "world/editor_world_input_controller.hpp"
#include "assets/editor_asset_spawn.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_project.hpp"
#include "editor_surface_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "world/editor_world.hpp"

#include <sfg/data/frame_hash_map.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
#define EDITOR_WORLD_INPUT_CAMERA_BASE_MOVE_SPEED  12.0f
#define EDITOR_WORLD_INPUT_CAMERA_BOOST_MULTIPLIER 8.0f
#define EDITOR_WORLD_INPUT_FOCUS_POINT_HALF_EXTENT 0.5f
#define EDITOR_WORLD_INPUT_VISIBILITY_CAPACITY	   64

	void editor_world_input_controller_t::init(editor_world_t& world, editor_world_handle_t handle)
	{
		_show_alone_states.reserve(EDITOR_WORLD_INPUT_VISIBILITY_CAPACITY);

		_world	= &world;
		_handle = handle;
	}

	void editor_world_input_controller_t::uninit()
	{
		deactivate();

		_show_alone_states.resize(0);
		_world				   = nullptr;
		_handle				   = {};
		_visibility_generation = 0;
		_show_alone_active	   = false;
	}

	void editor_world_input_controller_t::deactivate()
	{
		cancel_gizmo_action();
		end_camera_control();
	}

	void editor_world_input_controller_t::tick(const vec2f_t& relative_position, bool hovered)
	{
		if (!_gizmo_press_consumed && !_shoot_ray_press_consumed && hovered)
			_world->update_gizmo_hover(relative_position);
	}

	void editor_world_input_controller_t::pointer_press(const vec2f_t& relative_position, editor_world_input_pointer_button_e button)
	{
		SFG_ASSERT(s_event_runtime != nullptr);

		if (button == editor_world_input_pointer_button_e::right)
		{
			begin_camera_control(*s_event_runtime);
			return;
		}

		const editor_play_mode_e play_mode		   = _world->get_edit_context().get_play_mode();
		const bool				 physics_play_mode = play_mode == editor_play_mode_e::play_physics || play_mode == editor_play_mode_e::play_physics_paused;

		if (physics_play_mode && _world->get_edit_context().is_shoot_rays_enabled())
		{
			_shoot_ray_press_consumed = true;
			return;
		}

		_gizmo_press_consumed = _world->begin_gizmo_action(relative_position);
	}

	void editor_world_input_controller_t::pointer_release(const vec2f_t& relative_position, editor_world_input_pointer_button_e button, bool hovered)
	{
		if (button == editor_world_input_pointer_button_e::right)
		{
			end_camera_control();
			return;
		}

		if (_shoot_ray_press_consumed)
		{
			_shoot_ray_press_consumed = false;
			_world->shoot_ray_from_camera(relative_position);
			return;
		}

		if (_gizmo_press_consumed)
		{
			_world->update_gizmo_action(relative_position);
			_world->end_gizmo_action();
			_world->update_gizmo_hover(relative_position);
			_gizmo_press_consumed = false;
			return;
		}

		if (hovered)
		{
			const bool incremental_selection = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

			_world->request_entity_pick(relative_position, incremental_selection);
		}
	}

	void editor_world_input_controller_t::pointer_hover_move(const vec2f_t& relative_position)
	{
		if (!_gizmo_press_consumed && !_shoot_ray_press_consumed)
			_world->update_gizmo_hover(relative_position);
	}

	void editor_world_input_controller_t::pointer_hover_exit()
	{
		_world->clear_gizmo_hover();
	}

	void editor_world_input_controller_t::pointer_drag(const vec2f_t& relative_position)
	{
		if (_gizmo_press_consumed)
			_world->update_gizmo_action(relative_position);
	}

	void editor_world_input_controller_t::focus_lost()
	{
		deactivate();
	}

	void editor_world_input_controller_t::key_press(u16 key)
	{
		if (key == static_cast<u16>(input_code::key_escape))
		{
			if (_gizmo_press_consumed)
				cancel_gizmo_action();

			return;
		}

		const bool ctrl_pressed = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (key == static_cast<u16>(input_code::key_x) || (key == static_cast<u16>(input_code::key_d) && ctrl_pressed))
		{
			cancel_gizmo_action();

			const span_t<const entity_id_t> selected = _world->get_edit_context().get_selected_entities();

			if (selected.size == 0)
				return;

			frame_vector_t<entity_id_t> entities = {};

			entities.resize(selected.size);
			entities.resize(_world->get_edit_context().collect_selected_mutable_root_entities(_world->get_world(), {.data = entities.data(), .size = entities.size()}));

			if (key == static_cast<u16>(input_code::key_x))
			{
				editor_commands_entity_t::destroy(_handle, entities);
			}
			else
			{
				frame_vector_t<entity_id_t> duplicates = {};

				editor_commands_entity_t::duplicate(_handle, entities, duplicates);
			}

			return;
		}

		if (key == static_cast<u16>(input_code::key_f))
		{
			focus_selection();
			return;
		}

		if (key == static_cast<u16>(input_code::key_h))
		{
			cancel_gizmo_action();

			if (ctrl_pressed)
				toggle_show_alone();
			else
				hide_selection();

			return;
		}

		editor_world_edit_context_t& context = _world->get_edit_context();

		if (key == static_cast<u16>(input_code::key_alpha1))
		{
			cancel_gizmo_action();
			context.set_transform_control_type(editor_transform_control_type_e::move);
		}
		else if (key == static_cast<u16>(input_code::key_alpha2))
		{
			cancel_gizmo_action();
			context.set_transform_control_type(editor_transform_control_type_e::rotate);
		}
		else if (key == static_cast<u16>(input_code::key_alpha3))
		{
			cancel_gizmo_action();
			context.set_transform_control_type(editor_transform_control_type_e::scale);
		}
		else if (key == static_cast<u16>(input_code::key_alpha4))
		{
			cancel_gizmo_action();
			context.set_transform_locality(context.get_transform_locality() == editor_transform_locality_e::world ? editor_transform_locality_e::local : editor_transform_locality_e::world);
		}
		else if (key == static_cast<u16>(input_code::key_alpha5))
		{
			context.set_transform_snapping(context.get_transform_snapping() == editor_transform_snapping_e::default_ ? editor_transform_snapping_e::none : editor_transform_snapping_e::default_);

			editor_project_t& project					 = editor_project_t::get();
			project.settings.world_view_snapping_enabled = context.get_transform_snapping() == editor_transform_snapping_e::default_;
			project.save(project._runtime.path.c_str());
		}
	}

	void editor_world_input_controller_t::wheel(f32 delta)
	{
		pass_camera_input({.wheel_delta = delta});
	}

	void editor_world_input_controller_t::hide_selection()
	{
		hide(_world->get_edit_context().get_selected_entities());
	}

	void editor_world_input_controller_t::hide(entity_id_t entity)
	{
		hide({.data = &entity, .size = 1});
	}

	void editor_world_input_controller_t::toggle_show_alone()
	{
		world_t&			   world		  = _world->get_world();
		ecs_component_table_t& disabled_table = world.get_component_table(type_id_t<component_disabled_t>::value);

		if (_show_alone_active)
		{
			bool changed = false;

			for (const show_alone_entity_state_t& state : _show_alone_states)
			{
				const entity_id_t entity = world.find_by_guid(state.entity);

				if (entity == NULL_ENTITY_ID)
					continue;

				const bool disabled = ecs_t::table_has(disabled_table, entity);

				if (disabled == state.disabled)
					continue;

				if (state.disabled)
					ecs_t::table_add(disabled_table, entity);
				else
					ecs_t::table_remove(disabled_table, entity);

				changed = true;
			}

			_show_alone_states.resize(0);
			_show_alone_active = false;

			if (changed)
				++_visibility_generation;

			return;
		}

		const span_t<const entity_id_t> selected = _world->get_edit_context().get_selected_entities();

		if (selected.size == 0)
			return;

		frame_hash_map_t<entity_id_t, bool> selection_states = {};
		frame_vector_t<entity_id_t>			entities		 = {};

		selection_states.reserve(selected.size);
		entities.reserve(selected.size);

		for (size_t i = 0; i < selected.size; ++i)
		{
			if (selection_states.emplace(selected.data[i], true).second)
				entities.push_back(selected.data[i]);
		}

		for (const reflected_type_t& type : reflection_registry_t::get().get_types())
		{
			if (!type.flags.is_set(reflected_type_flag_renderable))
				continue;

			const ecs_component_table_t* table = world.find_component_table(type.type_id);

			if (table == nullptr)
				continue;

			const ecs_component_table_ref_t table_refs[] = {table->ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				if (selection_states.emplace(row.id, false).second)
					entities.push_back(row.id);
			}
		}

		_show_alone_states.resize(0);
		_show_alone_states.reserve(entities.size());

		for (const entity_id_t entity : entities)
		{
			const bool disabled		   = ecs_t::table_has(disabled_table, entity);
			const bool target_disabled = !selection_states.find(entity)->second;

			if (disabled == target_disabled)
				continue;

			_show_alone_states.push_back({
				.entity	  = world.get_entity_guid(entity),
				.disabled = disabled,
			});

			if (target_disabled)
				ecs_t::table_add(disabled_table, entity);
			else
				ecs_t::table_remove(disabled_table, entity);
		}

		_show_alone_active = true;

		if (!_show_alone_states.empty())
			++_visibility_generation;
	}

	bool editor_world_input_controller_t::payload_drop(const editor_payload_t& payload, const vec2f_t& screen_position)
	{
		if (payload.type != editor_payload_type_e::asset && payload.type != editor_payload_type_e::asset_multi)
			return false;

		return editor_asset_spawn_t::spawn_from_payload({
			.payload	= &payload,
			.screen_pos = screen_position,
			.world		= _handle,
			.parent		= NULL_ENTITY_ID,
		});
	}

	bool editor_world_input_controller_t::on_window_event(window_runtime_t& runtime, const window_event_t& ev)
	{
		s_event_runtime = &runtime;

		if (ev.type == window_event_type_e::focus && ev.sub_type == window_event_sub_type_e::release)
		{
			reset_camera_input(runtime);
			return false;
		}

		editor_world_input_controller_t* const active = s_active_camera_controller;

		if (active == nullptr || active->_camera_runtime != &runtime || !active->_camera_control)
			return false;

		if (ev.type == window_event_type_e::delta)
		{
			active->pass_camera_input({.mouse_delta = {static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y)}});
			return true;
		}

		if (ev.type == window_event_type_e::key)
			return active->pass_camera_key(ev);

		return false;
	}

	void editor_world_input_controller_t::reset_camera_input(window_runtime_t& runtime)
	{
		if (s_active_camera_controller != nullptr && s_active_camera_controller->_camera_runtime == &runtime)
			s_active_camera_controller->end_camera_control();
	}

	void editor_world_input_controller_t::begin_camera_control(window_runtime_t& runtime)
	{
		const editor_play_mode_e play_mode = _world->get_edit_context().get_play_mode();

		if (play_mode == editor_play_mode_e::play || play_mode == editor_play_mode_e::play_paused)
			return;

		if (s_active_camera_controller != nullptr && s_active_camera_controller != this)
			s_active_camera_controller->end_camera_control();

		editor_surface_controller_t::get().begin_editor_camera_cursor_capture(runtime);
		_camera_runtime			   = &runtime;
		_camera_control			   = true;
		s_active_camera_controller = this;

		pass_camera_input({.reset = true});
	}

	void editor_world_input_controller_t::end_camera_control()
	{
		if (!_camera_control)
			return;

		editor_surface_controller_t::get().end_editor_camera_cursor_capture(*_camera_runtime);
		pass_camera_input({.reset = true});

		_camera_runtime = nullptr;
		_camera_control = false;

		if (s_active_camera_controller == this)
			s_active_camera_controller = nullptr;
	}

	void editor_world_input_controller_t::pass_camera_input(const editor_world_camera_input_t& input)
	{
		_world->pass_camera_input(input);
	}

	bool editor_world_input_controller_t::pass_camera_key(const window_event_t& ev)
	{
		if (ev.sub_type != window_event_sub_type_e::press && ev.sub_type != window_event_sub_type_e::release)
			return false;

		const f32					sign  = ev.sub_type == window_event_sub_type_e::press ? 1.0f : -1.0f;
		editor_world_camera_input_t input = {};

		if (ev.button == static_cast<u16>(input_code::key_w))
			input.direction_delta.z = sign;
		else if (ev.button == static_cast<u16>(input_code::key_s))
			input.direction_delta.z = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_d))
			input.direction_delta.x = sign;
		else if (ev.button == static_cast<u16>(input_code::key_a))
			input.direction_delta.x = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_e))
			input.direction_delta.y = sign;
		else if (ev.button == static_cast<u16>(input_code::key_q))
			input.direction_delta.y = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift))
		{
			input.set_move_speed = true;
			input.move_speed	 = ev.sub_type == window_event_sub_type_e::press ? EDITOR_WORLD_INPUT_CAMERA_BASE_MOVE_SPEED * EDITOR_WORLD_INPUT_CAMERA_BOOST_MULTIPLIER : EDITOR_WORLD_INPUT_CAMERA_BASE_MOVE_SPEED;
		}
		else
			return false;

		pass_camera_input(input);
		return true;
	}

	void editor_world_input_controller_t::cancel_gizmo_action()
	{
		_world->cancel_gizmo_action();
		_world->clear_gizmo_hover();

		_gizmo_press_consumed	  = false;
		_shoot_ray_press_consumed = false;
	}

	void editor_world_input_controller_t::focus_selection()
	{
		const span_t<const entity_id_t> selected = _world->get_edit_context().get_selected_entities();

		if (selected.size == 0)
			return;

		const world_t&				 world					= _world->get_world();
		const ecs_component_table_t& transform_table		= world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& mesh_renderer_table	= world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t& skinned_renderer_table = world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		resource_manager_t&			 resource_manager		= resource_manager_t::get();
		vec3f_t						 bounds_min				= vec3f_t::zero;
		vec3f_t						 bounds_max				= vec3f_t::zero;
		bool						 has_bounds				= false;

		for (size_t entity_index = 0; entity_index < selected.size; ++entity_index)
		{
			const entity_id_t					entity	  = selected.data[entity_index];
			const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, entity);
			resource_handle_t					mesh	  = NULL_RESOURCE_HANDLE;

			if (const component_mesh_renderer_t* renderer = ecs_helpers_t::table_find_as_const<component_mesh_renderer_t>(mesh_renderer_table, entity))
				mesh = renderer->mesh;
			else if (const component_skinned_mesh_renderer_t* renderer = ecs_helpers_t::table_find_as_const<component_skinned_mesh_renderer_t>(skinned_renderer_table, entity))
				mesh = renderer->mesh;

			const mesh_internals_t* mesh_internals = mesh == NULL_RESOURCE_HANDLE ? nullptr : resource_manager.find_internals<mesh_internals_t>(mesh);

			if (mesh_internals == nullptr)
			{
				const vec3f_t point_min = transform.abs_pos - vec3f_t(EDITOR_WORLD_INPUT_FOCUS_POINT_HALF_EXTENT);
				const vec3f_t point_max = transform.abs_pos + vec3f_t(EDITOR_WORLD_INPUT_FOCUS_POINT_HALF_EXTENT);

				if (!has_bounds)
				{
					bounds_min = point_min;
					bounds_max = point_max;
					has_bounds = true;
				}
				else
				{
					bounds_min = vec3f_t::min(bounds_min, point_min);
					bounds_max = vec3f_t::max(bounds_max, point_max);
				}

				continue;
			}

			for (u8 corner_index = 0; corner_index < 8; ++corner_index)
			{
				const vec3f_t local_corner{
					(corner_index & 1) != 0 ? mesh_internals->local_bounds.bounds_max.x : mesh_internals->local_bounds.bounds_min.x,
					(corner_index & 2) != 0 ? mesh_internals->local_bounds.bounds_max.y : mesh_internals->local_bounds.bounds_min.y,
					(corner_index & 4) != 0 ? mesh_internals->local_bounds.bounds_max.z : mesh_internals->local_bounds.bounds_min.z,
				};
				const vec3f_t world_corner = transform.abs_mat * local_corner;

				if (!has_bounds)
				{
					bounds_min = world_corner;
					bounds_max = world_corner;
					has_bounds = true;
				}
				else
				{
					bounds_min = vec3f_t::min(bounds_min, world_corner);
					bounds_max = vec3f_t::max(bounds_max, world_corner);
				}
			}
		}

		_world->fit_camera_to_bounds(aabb_t(bounds_min, bounds_max));
	}

	void editor_world_input_controller_t::hide(span_t<const entity_id_t> entities)
	{
		if (entities.size == 0)
			return;

		world_t&					 world			  = _world->get_world();
		editor_world_edit_context_t& context		  = _world->get_edit_context();
		ecs_component_table_t&		 disabled_table	  = world.get_component_table(type_id_t<component_disabled_t>::value);
		frame_vector_t<entity_id_t>	 mutable_entities = {};

		mutable_entities.resize(entities.size);
		mutable_entities.resize(context.collect_mutable_entities(world, entities, {.data = mutable_entities.data(), .size = mutable_entities.size()}));

		for (const entity_id_t entity : mutable_entities)
		{
			if (ecs_t::table_has(disabled_table, entity))
				ecs_t::table_remove(disabled_table, entity);
			else
				ecs_t::table_add(disabled_table, entity);
		}

		if (!mutable_entities.empty())
			++_visibility_generation;
	}
}
