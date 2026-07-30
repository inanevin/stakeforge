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

#include "world.hpp"
#include "world_debug_draw.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/prefab.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/resources/world_cook_entity_header.hpp>
#include <sfg/runtime/scripting/script_component_schema.hpp>
#include <sfg/runtime/scripting/script_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
	world_t::world_t()	= default;
	world_t::~world_t() = default;

	void world_t::init(const world_init_config_t& config)
	{
		SFG_ASSERT(config.particle_simulation.particle_max_count == config.render_particle_max_count);

		_debug_draw.init(config.debug_draw);
		_component_tables.reserve(config.component_table_initial_capacity);
		_entity_free_list.reserve(config.entity_free_list_initial_capacity);
		_text_allocations.reserve(config.text_allocation_initial_capacity);
		_text_allocation_free_list.reserve(config.text_allocation_initial_capacity);
		_text_allocator.init(config.text_budget_bytes);
		_used_resources.reserve(config.used_resource_initial_capacity);
		_screen.init(config.render_resolution);

		const vector_t<reflected_type_t>& types = reflection_registry_t::get().get_types();

		for (const reflected_type_t& type : types)
		{
			const bool is_component		   = type.flags.is_set(reflected_type_flags_e::reflected_type_flag_component);
			const bool is_tag_component	   = type.flags.is_set(reflected_type_flags_e::reflected_type_flag_tag_component);
			const bool is_system_component = type.flags.is_set(reflected_type_flags_e::reflected_type_flag_system_component);

			if (!is_component && !is_tag_component && !is_system_component)
				continue;

			add_component_table(ecs_helpers_t::make_component_desc(type.type_id, is_tag_component ? 0 : type.size, is_tag_component ? 1 : type.alignment, is_tag_component ? ecs_component_type_flags_tag : ecs_component_type_flags_none, type.name));
		}

		refresh_component_table_cache();

		_logic_helper.init(*this);

		_animation_controller.init(*this, config.render_bone_max_count, config.animation_graph_budget_bytes);

		_audio_controller.init(*this);

		_canvas_controller.init(*this);

		_particle_simulation.init(*this, config.particle_simulation);

		if (config.physics_enabled)
		{
			_physics_world.init(*this, config.physics);
		}
	}

	void world_t::uninit()
	{
		SFG_ASSERT(!_is_playing);
		SFG_ASSERT(_active_component_query_count == 0);
		SFG_ASSERT(_world_script_instance == nullptr);

		_particle_simulation.uninit();

		_canvas_controller.uninit();

		_screen.uninit();

		_audio_controller.uninit();

		_animation_controller.uninit();

		if (_physics_world.is_init())
		{
			_physics_world.uninit();
		}

		_logic_helper.uninit();

		_debug_draw.uninit();

		for (ecs_component_table_t& table : _component_tables)
			ecs_t::table_uninit(table);

		_used_resources.resize(0);
		_component_tables.resize(0);
		_entity_free_list.resize(0);
		_text_allocations.resize(0);
		_text_allocation_free_list.resize(0);
		_text_allocator.uninit();
		_engine_components			  = {};
		_system_components			  = {};
		_tick_count					  = 0;
		_entity_head				  = 0;
		_main_camera_entity			  = NULL_ENTITY_ID;
		_play_resource_count		  = 0;
		_active_component_query_count = 0;
		_world_script_instance		  = nullptr;
		_elapsed_time				  = 0.0f;
		_real_elapsed_time			  = 0.0f;
		_is_playing					  = false;
		_time_scale					  = 1.0f;

		for (key_state_t& state : _key_states)
			state = {};
	}

	void world_t::begin_play()
	{
		SFG_ASSERT(!_is_playing);

		_play_resource_count = static_cast<u32>(_used_resources.size());
		_is_playing			 = true;
		_elapsed_time		 = 0.0f;
		_real_elapsed_time	 = 0.0f;

		update_world_transforms(false);
		refresh_main_camera();
		_audio_controller.set_time_scale(_time_scale);
		_audio_controller.begin_play();
		_particle_simulation.begin_play();
		_canvas_controller.sync_create_destroy_canvases();

		if (_physics_world.is_init())
			_physics_world.sync_body_create_destroy();

		begin_world_script_play();
	}

	void world_t::end_play()
	{
		SFG_ASSERT(_is_playing);

		flush_pressed_keys();
		end_world_script_play();
		_canvas_controller.clear_widgets();

		_audio_controller.end_play();
		_particle_simulation.end_play();

		if (_physics_world.is_init())
			_physics_world.clear();

		resource_manager_t& resource_manager = resource_manager_t::get();
		for (size_t i = _used_resources.size(); i > _play_resource_count; --i)
		{
			world_resource_t& resource = _used_resources[i - 1];
			if (resource.loaded)
				resource_manager.unload_resource(resource.handle, false);
		}
		_used_resources.resize(_play_resource_count);
		_play_resource_count = 0;
		_is_playing			 = false;

		set_time_scale(1.0f);
	}

	void world_t::pause_audio()
	{
		SFG_ASSERT(_is_playing);

		_audio_controller.pause_all();
	}

	void world_t::resume_audio()
	{
		SFG_ASSERT(_is_playing);

		_audio_controller.resume_all();
	}

	void world_t::set_time_scale(f32 time_scale)
	{
		_time_scale = math::max(0.0f, time_scale);

		_audio_controller.set_time_scale(_time_scale);
	}

	void world_t::reset_world_state()
	{
		SFG_ASSERT(!_is_playing);

		if (_physics_world.is_init())
			_physics_world.clear();

		_particle_simulation.clear();
		_animation_controller.clear();
		_audio_controller.clear();
		_canvas_controller.clear();

		_debug_draw.begin_frame();

		for (ecs_component_table_t& table : _component_tables)
			ecs_t::table_clear(table);

		_text_allocations.resize(0);
		_text_allocation_free_list.resize(0);
		_entity_free_list.resize(0);
		_text_allocator.reset();
		_tick_count			= 0;
		_elapsed_time		= 0.0f;
		_real_elapsed_time	= 0.0f;
		_time_scale			= 1.0f;
		_entity_head		= 0;
		_main_camera_entity = NULL_ENTITY_ID;
		_screen.clear_camera();
		_audio_controller.set_time_scale(_time_scale);

		for (key_state_t& state : _key_states)
			state = {};
	}

	void world_t::tick_physics(f32 dt)
	{
		ZoneScoped;

		if (!_physics_world.is_init())
			return;

		const f32 scaled_dt = dt * _time_scale;

		_physics_world.tick(scaled_dt);
	}

	void world_t::tick_animation_prep(f32 dt)
	{
		ZoneScoped;

		const f32 scaled_dt = dt * _time_scale;

		_animation_controller.tick_prep(scaled_dt);
	}

	void world_t::tick_animation_logic(f32 dt)
	{
		ZoneScoped;

		const f32 scaled_dt = dt * _time_scale;

		_animation_controller.tick_logic(scaled_dt);
	}

	void world_t::tick_logic(f32 dt)
	{
		ZoneScoped;

		SFG_ASSERT(_is_playing);

		const f32 scaled_dt = dt * _time_scale;

		_elapsed_time += scaled_dt;
		_real_elapsed_time += dt;

		if (_world_script_instance != nullptr)
			script_runtime_t::get().tick_world_script(_world_script_instance, scaled_dt);

		_logic_helper.sync_destroyers(scaled_dt);
		_audio_controller.tick(scaled_dt);

		if (_world_script_instance != nullptr)
			script_runtime_t::get().post_tick_world_script(_world_script_instance, scaled_dt);
	}

	void world_t::tick_logic_post_physics(f32 dt)
	{
		ZoneScoped;

		SFG_ASSERT(_is_playing);

		if (_world_script_instance == nullptr)
			return;

		const f32		  scaled_dt		 = dt * _time_scale;
		script_runtime_t& script_runtime = script_runtime_t::get();

		if (_physics_world.is_init())
		{
			const span_t<const physics_contact_event_t> contact_events = _physics_world.get_contact_events();

			for (const physics_contact_event_t& contact : contact_events)
				script_runtime.physics_contact_world_script(_world_script_instance, contact);
		}

		script_runtime.post_physics_tick_world_script(_world_script_instance, scaled_dt);
	}

	void world_t::tick_logic_post_animation(f32 dt)
	{
		ZoneScoped;

		SFG_ASSERT(_is_playing);

		const f32 scaled_dt = dt * _time_scale;

		if (_world_script_instance != nullptr)
			script_runtime_t::get().post_animation_tick_world_script(_world_script_instance, scaled_dt);
	}

	void world_t::draw_world_script_debug()
	{
		ZoneScoped;

		if (_world_script_instance != nullptr)
			script_runtime_t::get().draw_debug_world_script(_world_script_instance);
	}

	void world_t::begin_world_script_play()
	{
		SFG_ASSERT(_is_playing);
		SFG_ASSERT(_world_script_instance == nullptr);

		script_runtime_t& script_runtime = script_runtime_t::get();

		if (!script_runtime.is_project_assembly_loaded())
			return;

		const ecs_component_table_ref_t table_refs[] = {
			_engine_components.world_script_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_world_script_t& world_script = *static_cast<const component_world_script_t*>(row.components[0]);

			if (world_script.script_type_id == 0 || world_script.script_type_id == NULL_SID || script_runtime.get_component_schema().find_world_script(world_script.script_type_id) == nullptr)
				return;

			void* const instance = script_runtime.create_world_script(world_script.script_type_id, this);

			if (instance == nullptr)
				return;

			_world_script_instance = instance;

			if (!script_runtime.begin_play_world_script(_world_script_instance))
			{
				script_runtime.destroy_world_script(_world_script_instance);
				_world_script_instance = nullptr;
			}

			return;
		}
	}

	void world_t::end_world_script_play()
	{
		SFG_ASSERT(_is_playing);

		if (_world_script_instance == nullptr)
			return;

		script_runtime_t& script_runtime = script_runtime_t::get();

		script_runtime.end_play_world_script(_world_script_instance);
		script_runtime.destroy_world_script(_world_script_instance);
		_world_script_instance = nullptr;
	}

	void world_t::key_event(u16 key, u16 scan_code, u8 action)
	{
		SFG_ASSERT(_is_playing);

		const bool consumed = _canvas_controller.key_event(key, scan_code, action);

		if (consumed)
			return;

		update_key_state(key, scan_code, action);

		if (_world_script_instance != nullptr)
			script_runtime_t::get().key_event_world_script(_world_script_instance, key, scan_code, action);
	}

	void world_t::focus_event(bool focused)
	{
		SFG_ASSERT(_is_playing);

		if (!focused)
			flush_pressed_keys();
	}

	void world_t::mouse_button_event(u8 button, u8 action, f32 position_x, f32 position_y)
	{
		SFG_ASSERT(_is_playing);

		const bool had_canvas_focus = _canvas_controller.is_keyboard_focus_active();
		const bool consumed			= _canvas_controller.mouse_button_event(button, action, {position_x, position_y});

		if (!had_canvas_focus && _canvas_controller.is_keyboard_focus_active())
			flush_pressed_keys();

		if (!consumed && _world_script_instance != nullptr)
			script_runtime_t::get().mouse_button_event_world_script(_world_script_instance, button, action, position_x, position_y);
	}

	void world_t::mouse_move_event(f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
		SFG_ASSERT(_is_playing);

		const bool consumed = _canvas_controller.mouse_move_event({position_x, position_y});

		if (!consumed && _world_script_instance != nullptr)
			script_runtime_t::get().mouse_move_event_world_script(_world_script_instance, position_x, position_y, delta_x, delta_y);
	}

	void world_t::mouse_wheel_event(f32 position_x, f32 position_y, f32 delta)
	{
		SFG_ASSERT(_is_playing);

		const bool consumed = _canvas_controller.mouse_wheel_event({position_x, position_y}, delta);

		if (!consumed && _world_script_instance != nullptr)
			script_runtime_t::get().mouse_wheel_event_world_script(_world_script_instance, position_x, position_y, delta);
	}

	void world_t::flush_pressed_keys()
	{
		for (u16 key = 0; key < std::size(_key_states); ++key)
		{
			key_state_t& state = _key_states[key];

			if (!state.pressed)
				continue;

			state.pressed = false;

			if (_world_script_instance != nullptr)
				script_runtime_t::get().key_event_world_script(_world_script_instance, key, state.scan_code, static_cast<u8>(window_event_sub_type_e::release));
		}
	}

	void world_t::update_key_state(u16 key, u16 scan_code, u8 action)
	{
		if (key >= std::size(_key_states))
			return;

		key_state_t& state = _key_states[key];

		if (action == static_cast<u8>(window_event_sub_type_e::press))
		{
			state.scan_code = scan_code;
			state.pressed	= true;
		}
		else if (action == static_cast<u8>(window_event_sub_type_e::release))
			state.pressed = false;
	}

	void world_t::tick_post(f32 dt)
	{
		ZoneScoped;

		++_tick_count;

		const f32 scaled_dt = dt * _time_scale;

		refresh_main_camera();

		_particle_simulation.tick(scaled_dt);
		_canvas_controller.tick(dt);
		_canvas_controller.dispatch_events(_world_script_instance);

		_logic_helper.sync_sprite_renderers();
		_logic_helper.sync_reflection_probes(_tick_count);
	}

	void world_t::refresh_main_camera()
	{
		_main_camera_entity = _logic_helper.sync_main_camera();

		if (_main_camera_entity == NULL_ENTITY_ID)
		{
			_screen.clear_camera();
			return;
		}

		const ecs_component_table_t&		camera_table	= get_component_table(type_id_t<component_camera_t>::value);
		const ecs_component_table_t&		transform_table = get_component_table(type_id_t<component_system_transform_t>::value);
		const component_camera_t&			camera			= ecs_helpers_t::table_get_as_const<component_camera_t>(camera_table, _main_camera_entity);
		const component_system_transform_t& transform		= ecs_helpers_t::table_get_as_const<component_system_transform_t>(transform_table, _main_camera_entity);
		const world_render_view_t			render_view{
			.pos		 = transform.abs_pos,
			.rot		 = transform.abs_rot,
			.prev_pos	 = transform.prev_abs_pos,
			.prev_rot	 = transform.prev_abs_rot,
			.near_plane	 = camera.near_plane,
			.far_plane	 = camera.far_plane,
			.fov_degrees = camera.fov_degrees,
		};

		_screen.update_camera(render_view);
	}

	void world_t::recreate_physical(entity_id_t id)
	{
		if (!_physics_world.is_init())
			return;

		_physics_world.destroy_body(id);
		_physics_world.sync_body_create_destroy();
	}

	entity_guid_t world_t::generate_guid() const
	{
		entity_guid_t guid = NULL_ENTITY_GUID;
		do
		{
			guid = hashing_t::generate_guid64();
		} while (guid == NULL_ENTITY_GUID || find_by_guid(guid) != NULL_ENTITY_ID);
		return guid;
	}

	entity_id_t world_t::create_entity(const char* name, entity_guid_t guid)
	{
		entity_id_t id = NULL_ENTITY_ID;
		if (!_entity_free_list.empty())
		{
			id = _entity_free_list.back();
			_entity_free_list.pop_back();
		}
		else
		{
			SFG_ASSERT(_entity_head < ECS_MAX_ENTITIES);
			id = _entity_head;
			_entity_head++;
		}

		if (guid == NULL_ENTITY_GUID)
		{
			guid = generate_guid();
		}
		else
		{
			SFG_ASSERT(find_by_guid(guid) == NULL_ENTITY_ID);
		}

		ecs_t::table_add(*_engine_components.alive_table, id);
		ecs_helpers_t::table_add_or_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);

		component_guid_t& guid_component = ecs_helpers_t::table_add_or_get_as<component_guid_t>(*_engine_components.guid_table, id);
		guid_component.guid				 = guid;

		ecs_helpers_t::table_add_or_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_name_t& name_component = ecs_helpers_t::table_add_or_get_as<component_name_t>(*_engine_components.name_table, id);

		if (name == nullptr)
			name_component.text[0] = '\0';
		else
		{
			const size_t name_len  = std::strlen(name);
			const size_t text_size = sizeof(name_component.text);
			const size_t copy_len  = name_len < text_size ? name_len : text_size - 1;
			SFG_MEMCPY((void*)name_component.text, name, copy_len);
			name_component.text[copy_len] = '\0';
		}

		component_system_transform_t& system_transform = ecs_helpers_t::table_add_or_get_as<component_system_transform_t>(*_system_components.transform_table, id);
		system_transform.snap_interpolation			   = true;

		return id;
	}

	void world_t::destroy_entity(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		_particle_simulation.destroy_entity(id);

		if (_physics_world.is_init())
			_physics_world.destroy_entity(id);

		_animation_controller.destroy_entity(id);
		_audio_controller.destroy_entity(id);
		_canvas_controller.destroy_entity(id);

		component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		SFG_ASSERT(hierarchy.first_child == NULL_ENTITY_ID);

		detach(id);

		for (ecs_component_table_t& t : _component_tables)
		{
			ecs_t::table_remove(t, id);
		}

		_entity_free_list.push_back(id);
	}

	void world_t::destroy_entity_tree(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
			destroy_entity_tree(child);
			child = next_child;
		}

		destroy_entity(id);
	}

	void world_t::set_entity_name(entity_id_t id, const char* name)
	{
		SFG_ASSERT(is_alive(id));

		component_name_t& name_component = ecs_helpers_t::table_get_as<component_name_t>(*_engine_components.name_table, id);

		if (name == nullptr)
			name_component.text[0] = '\0';
		else
		{
			const size_t name_len  = std::strlen(name);
			const size_t text_size = sizeof(name_component.text);
			const size_t copy_len  = name_len < text_size ? name_len : text_size - 1;
			SFG_MEMCPY((void*)name_component.text, name, copy_len);
			name_component.text[copy_len] = '\0';
		}
	}

	entity_id_t world_t::get_entity_parent(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		return hierarchy.parent;
	}

	entity_guid_t world_t::get_entity_guid(entity_id_t id) const
	{
		if (id == NULL_ENTITY_ID)
			return NULL_ENTITY_GUID;

		SFG_ASSERT(is_alive(id));
		const component_guid_t& guid = ecs_helpers_t::table_get_as_const<component_guid_t>(*_engine_components.guid_table, id);
		return guid.guid;
	}

	entity_id_t world_t::find_by_guid(entity_guid_t guid) const
	{
		if (guid == NULL_ENTITY_GUID)
			return NULL_ENTITY_ID;

		const ecs_component_table_ref_t table_refs[] = {
			_engine_components.alive_table->ref(),
			_engine_components.guid_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_guid_t& g = ecs_helpers_t::row_get<component_guid_t>(row, 1);
			if (g.guid == guid)
				return row.id;
		}

		return NULL_ENTITY_ID;
	}

	void world_t::attach_to(entity_id_t id, entity_id_t parent)
	{
		SFG_ASSERT(id != parent);
		SFG_ASSERT(is_alive(id));
		SFG_ASSERT(is_alive(parent));

		for (entity_id_t current = parent; current != NULL_ENTITY_ID;)
		{
			SFG_ASSERT(current != id);
			const component_hierarchy_t& current_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			current										   = current_hierarchy.parent;
		}

		component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		if (hierarchy.parent == parent)
			return;

		detach(id);

		component_hierarchy_t& parent_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, parent);
		component_hierarchy_t& child_hierarchy	= ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);

		child_hierarchy.parent		 = parent;
		child_hierarchy.prev_sibling = NULL_ENTITY_ID;
		child_hierarchy.next_sibling = NULL_ENTITY_ID;

		if (parent_hierarchy.first_child == NULL_ENTITY_ID)
		{
			parent_hierarchy.first_child = id;
			return;
		}

		entity_id_t last_child = parent_hierarchy.first_child;
		while (true)
		{
			component_hierarchy_t& last_child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, last_child);
			if (last_child_hierarchy.next_sibling == NULL_ENTITY_ID)
			{
				last_child_hierarchy.next_sibling = id;
				child_hierarchy.prev_sibling	  = last_child;
				break;
			}

			last_child = last_child_hierarchy.next_sibling;
		}
	}

	void world_t::detach(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		component_hierarchy_t& hierarchy	= ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		const entity_id_t	   parent		= hierarchy.parent;
		const entity_id_t	   next_sibling = hierarchy.next_sibling;
		const entity_id_t	   prev_sibling = hierarchy.prev_sibling;

		if (parent != NULL_ENTITY_ID)
		{
			component_hierarchy_t& parent_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, parent);
			if (parent_hierarchy.first_child == id)
				parent_hierarchy.first_child = next_sibling;
		}

		if (prev_sibling != NULL_ENTITY_ID)
		{
			component_hierarchy_t& prev_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, prev_sibling);
			prev_hierarchy.next_sibling			  = next_sibling;
		}

		if (next_sibling != NULL_ENTITY_ID)
		{
			component_hierarchy_t& next_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, next_sibling);
			next_hierarchy.prev_sibling			  = prev_sibling;
		}

		hierarchy.parent	   = NULL_ENTITY_ID;
		hierarchy.next_sibling = NULL_ENTITY_ID;
		hierarchy.prev_sibling = NULL_ENTITY_ID;
	}

	void world_t::set_entity_pos_local(entity_id_t id, const vec3f_t& pos)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.pos					 = pos;
	}

	void world_t::set_entity_rot_local(entity_id_t id, const quat_t& rot)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.rot					 = rot;
	}

	void world_t::set_entity_scale_local(entity_id_t id, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.scale					 = scale;
	}

	void world_t::teleport_entity(entity_id_t id, const vec3f_t& pos, const quat_t& rot, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		const vec3f_t local_pos	  = abs_pos_to_local(id, pos);
		const quat_t  local_rot	  = abs_rot_to_local(id, rot);
		const vec3f_t local_scale = abs_scale_to_local(id, scale);

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.pos					 = local_pos;
		transform.rot					 = local_rot;
		transform.scale					 = local_scale;

		set_entity_snap_interpolation_recursive(id);
	}

	void world_t::mark_entity_teleported(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));
		set_entity_snap_interpolation_recursive(id);
	}

	const vec3f_t& world_t::get_entity_pos_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.pos;
	}

	const quat_t& world_t::get_entity_rot_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.rot;
	}

	const vec3f_t& world_t::get_entity_scale_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.scale;
	}

	const vec3f_t& world_t::get_entity_pos_last_abs(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(*_system_components.transform_table, id);
		return transform.prev_abs_pos;
	}

	const quat_t& world_t::get_entity_rot_last_abs(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(*_system_components.transform_table, id);
		return transform.prev_abs_rot;
	}

	const vec3f_t& world_t::get_entity_scale_last_abs(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_system_transform_t& transform = ecs_helpers_t::table_get_as_const<component_system_transform_t>(*_system_components.transform_table, id);
		return transform.prev_abs_scale;
	}

	vec3f_t world_t::abs_pos_to_local(entity_id_t id, const vec3f_t& pos)
	{
		SFG_ASSERT(is_alive(id));

		const mat4x3_t parent_abs_mat = calculate_parent_transform_direct(id);
		return parent_abs_mat.inverse() * pos;
	}

	quat_t world_t::abs_rot_to_local(entity_id_t id, const quat_t& rot)
	{
		SFG_ASSERT(is_alive(id));

		vec3f_t parent_abs_pos;
		quat_t	parent_abs_rot;
		vec3f_t parent_abs_scale;
		calculate_parent_transform_direct(id).decompose(parent_abs_pos, parent_abs_rot, parent_abs_scale);
		return parent_abs_rot.inverse() * rot;
	}

	vec3f_t world_t::abs_scale_to_local(entity_id_t id, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		vec3f_t parent_abs_pos;
		quat_t	parent_abs_rot;
		vec3f_t parent_abs_scale;
		calculate_parent_transform_direct(id).decompose(parent_abs_pos, parent_abs_rot, parent_abs_scale);
		return {scale.x / parent_abs_scale.x, scale.y / parent_abs_scale.y, scale.z / parent_abs_scale.z};
	}

	mat4x3_t world_t::calculate_transform_direct(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		const mat4x3_t				  parent_abs_mat   = calculate_parent_transform_direct(id);
		const component_transform_t&  transform		   = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);

		system_transform.abs_mat = parent_abs_mat * mat4x3_t::transform(transform.pos, transform.rot, transform.scale);
		system_transform.abs_mat.decompose(system_transform.abs_pos, system_transform.abs_rot, system_transform.abs_scale);
		return system_transform.abs_mat;
	}

	void world_t::update_world_transforms(bool advance_interpolation)
	{
		ZoneScoped;

		const ecs_component_table_ref_t table_refs[] = {
			_engine_components.transform_table->ref(),
			_engine_components.hierarchy_table->ref(),
			_system_components.transform_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::row_get<component_hierarchy_t>(row, 1);

			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			update_entity_transform(row.id, hierarchy, vec3f_t::zero, quat_t::identity, vec3f_t::one, mat4x3_t::identity, advance_interpolation);
		}
	}

	void world_t::sync_entity_hierarchy(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		if (_physics_world.is_init())
			_physics_world.sync_body_create_destroy();
	}

	bool world_t::add_resource(resource_type_e type, resource_handle_t handle)
	{
		auto it = std::find_if(_used_resources.begin(), _used_resources.end(), [handle](const world_resource_t& r) -> bool { return r.handle == handle; });
		if (it != _used_resources.end())
			return false;
		if (handle == 0)
			return false;
		_used_resources.push_back({.handle = handle, .type = type});
		return true;
	}

	void world_t::scan_for_resources(entity_id_t entity, bool omit_children)
	{
		SFG_ASSERT(is_alive(entity));

		reflection_registry_t& registry = reflection_registry_t::get();
		bool				   added	= false;

		const auto scan_reflected_object = [&](const auto& self, const reflected_type_t& type, u8* object) -> void {
			for (u32 i = type.fields.start; i < type.fields.end; ++i)
			{
				const reflected_field_t* field = registry.get_field(i);
				SFG_ASSERT(field != nullptr);

				u8* field_ptr = object + field->offset;

				if (field->value_type == reflected_value_type_e::u64)
				{
					const resource_type_e resource_type = resource_type_from_reflection_sub_type_id(field->sub_type_id);

					if (resource_type == resource_type_e::invalid)
						continue;

					const resource_handle_t handle = *reinterpret_cast<const resource_handle_t*>(field_ptr);

					if (handle != NULL_RESOURCE_HANDLE)
						added = add_resource(resource_type, handle) || added;

					continue;
				}

				if (field->value_type == reflected_value_type_e::object)
				{
					const reflected_type_t* object_type = registry.find_type(field->sub_type_id);
					SFG_ASSERT(object_type != nullptr);

					self(self, *object_type, field_ptr);
					continue;
				}

				if (field->value_type != reflected_value_type_e::container)
					continue;

				const reflected_value_type_e element_value_type = field->container_ops.element_value_type;

				if (element_value_type != reflected_value_type_e::u64 && element_value_type != reflected_value_type_e::object)
					continue;

				const u32 element_count = static_cast<u32>(field->container_ops.get_element_size_fn(field_ptr));

				if (element_value_type == reflected_value_type_e::u64)
				{
					const resource_type_e resource_type = resource_type_from_reflection_sub_type_id(field->container_ops.element_sub_type_id);

					if (resource_type == resource_type_e::invalid)
						continue;

					for (u32 j = 0; j < element_count; ++j)
					{
						const resource_handle_t handle = *reinterpret_cast<const resource_handle_t*>(field->container_ops.get_element_ptr_fn(field_ptr, j));

						if (handle != NULL_RESOURCE_HANDLE)
							added = add_resource(resource_type, handle) || added;
					}

					continue;
				}

				const reflected_type_t* element_type = registry.find_type(field->container_ops.element_sub_type_id);
				SFG_ASSERT(element_type != nullptr);

				for (u32 j = 0; j < element_count; ++j)
					self(self, *element_type, field->container_ops.get_element_ptr_fn(field_ptr, j));
			}
		};

		const auto scan = [&](const auto& self, entity_id_t current) -> void {
			for (ecs_component_table_t& component_table : _component_tables)
			{
				if (!ecs_t::table_has(component_table, current))
					continue;

				const reflected_type_t* type = registry.find_type(component_table.component_type_id);
				if (type == nullptr || type->fields.start == type->fields.end)
					continue;

				void* component = ecs_t::table_get(component_table, current);
				SFG_ASSERT(component != nullptr);

				scan_reflected_object(scan_reflected_object, *type, static_cast<u8*>(component));
			}

			if (omit_children)
				return;

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				self(self, child);
				child = next_child;
			}
		};

		scan(scan, entity);

		if (added)
			load_all_used_resources();
	}

	void world_t::load_all_used_resources()
	{
		resource_manager_t& rm = resource_manager_t::get();
		for (world_resource_t& res : _used_resources)
		{
			if (res.loaded)
				continue;

			res.loaded = rm.load_resource(res.handle, res.type) != resource_state_e::failed;
		}
	}

	void world_t::unload_all_used_resources()
	{
		resource_manager_t& rm = resource_manager_t::get();

		for (auto it = _used_resources.rbegin(); it != _used_resources.rend(); ++it)
		{
			if (!it->loaded)
				continue;

			rm.unload_resource(it->handle, false);
			it->loaded = false;
		}
	}

	void world_t::clear_used_resources()
	{
		SFG_ASSERT(!_is_playing);

		unload_all_used_resources();
		_used_resources.resize(0);
	}

	void world_t::update_entity_transform(entity_id_t id, const component_hierarchy_t& own_hierarchy, const vec3f_t& parent_abs_pos, const quat_t& parent_abs_rot, const vec3f_t& parent_abs_scale, const mat4x3_t& parent_abs_mat, bool advance_interpolation)
	{
		const component_transform_t&  transform		   = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);

		if (advance_interpolation)
		{
			system_transform.prev_abs_mat	= system_transform.abs_mat;
			system_transform.prev_abs_rot	= system_transform.abs_rot;
			system_transform.prev_abs_pos	= system_transform.abs_pos;
			system_transform.prev_abs_scale = system_transform.abs_scale;
		}

		system_transform.abs_pos   = parent_abs_pos + (parent_abs_rot * (transform.pos * parent_abs_scale));
		system_transform.abs_rot   = parent_abs_rot * transform.rot;
		system_transform.abs_scale = parent_abs_scale * transform.scale;
		system_transform.abs_mat   = parent_abs_mat * mat4x3_t::transform(transform.pos, transform.rot, transform.scale);

		if (system_transform.snap_interpolation)
		{
			system_transform.prev_abs_mat		= system_transform.abs_mat;
			system_transform.prev_abs_rot		= system_transform.abs_rot;
			system_transform.prev_abs_pos		= system_transform.abs_pos;
			system_transform.prev_abs_scale		= system_transform.abs_scale;
			system_transform.snap_interpolation = false;
		}

		for (entity_id_t child = own_hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			update_entity_transform(child, child_hierarchy, system_transform.abs_pos, system_transform.abs_rot, system_transform.abs_scale, system_transform.abs_mat, advance_interpolation);
			child = child_hierarchy.next_sibling;
		}
	}

	void world_t::set_entity_snap_interpolation_recursive(entity_id_t id)
	{
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);
		system_transform.snap_interpolation			   = true;

		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			set_entity_snap_interpolation_recursive(child);
			child = child_hierarchy.next_sibling;
		}
	}

	mat4x3_t world_t::calculate_parent_transform_direct(entity_id_t id)
	{
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		if (hierarchy.parent == NULL_ENTITY_ID)
			return mat4x3_t::identity;

		return calculate_transform_direct(hierarchy.parent);
	}

	ecs_component_table_t& world_t::add_component_table(const ecs_component_type_desc_t& desc)
	{
		SFG_ASSERT(find_component_table(desc.type_id) == nullptr);
		SFG_ASSERT(!is_component_query_active());

		ecs_component_table_t& table = _component_tables.emplace_back();
		ecs_t::table_init(table, desc);
		return table;
	}

	void world_t::apply_script_component_schema(const script_component_schema_t& current_schema, const script_component_schema_t& candidate_schema, const script_component_schema_delta_t& delta)
	{
		SFG_ASSERT(!is_component_query_active());

		_component_tables.reserve(_component_tables.size() + delta.added.size());

		for (sid_t type_id : delta.removed)
		{
			const auto table_it = std::find_if(_component_tables.begin(), _component_tables.end(), [type_id](const ecs_component_table_t& table) { return table.component_type_id == type_id; });
			SFG_ASSERT(table_it != _component_tables.end());

			ecs_t::table_uninit(*table_it);
			_component_tables.erase(table_it);
		}

		for (sid_t type_id : delta.layout_changed)
		{
			const script_component_desc_t* current_component   = current_schema.find_component(type_id);
			const script_component_desc_t* candidate_component = candidate_schema.find_component(type_id);
			ecs_component_table_t*		   current_table	   = find_component_table(type_id);

			SFG_ASSERT(current_component != nullptr);
			SFG_ASSERT(candidate_component != nullptr);
			SFG_ASSERT(current_table != nullptr);

			ecs_component_table_t	candidate_table = {};
			const reflected_type_t* reflected_type	= reflection_registry_t::get().find_type(type_id);
			SFG_ASSERT(reflected_type != nullptr);

			const ecs_component_type_desc_t type_desc = ecs_helpers_t::make_component_desc(candidate_component->type_id, candidate_component->size, candidate_component->alignment, ecs_component_type_flags_none, reflected_type->name);
			ecs_t::table_init(candidate_table, type_desc);

			const ecs_component_table_ref_t table_refs[] = {
				current_table->ref(),
			};

			// replace data
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				void* candidate_data = ecs_t::table_add(candidate_table, row.id);

				SFG_MEMSET(candidate_data, 0, candidate_component->size);

				for (const script_component_field_desc_t& candidate_field : candidate_component->fields)
				{
					const script_component_field_desc_t* current_field = current_component->find_field(candidate_field.field_id);

					if (current_field == nullptr || current_field->value_type != candidate_field.value_type || current_field->sub_type_id != candidate_field.sub_type_id || current_field->size != candidate_field.size)
						continue;

					const u8* current_field_data   = static_cast<const u8*>(row.components[0]) + current_field->offset;
					u8*		  candidate_field_data = static_cast<u8*>(candidate_data) + candidate_field.offset;

					SFG_MEMCPY(candidate_field_data, current_field_data, candidate_field.size);
				}
			}

			ecs_t::table_uninit(*current_table);
			*current_table = candidate_table;
		}

		for (sid_t type_id : delta.added)
		{
			const script_component_desc_t* component	  = candidate_schema.find_component(type_id);
			const reflected_type_t*		   reflected_type = reflection_registry_t::get().find_type(type_id);

			SFG_ASSERT(component != nullptr);
			SFG_ASSERT(reflected_type != nullptr);

			add_component_table(ecs_helpers_t::make_component_desc(component->type_id, component->size, component->alignment, ecs_component_type_flags_none, reflected_type->name));
		}

		for (const script_component_desc_t& component : candidate_schema.get_components())
		{
			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(component.type_id);
			ecs_component_table_t*	table		   = find_component_table(component.type_id);

			SFG_ASSERT(reflected_type != nullptr);
			SFG_ASSERT(table != nullptr);

			table->type_desc = ecs_helpers_t::make_component_desc(component.type_id, component.size, component.alignment, ecs_component_type_flags_none, reflected_type->name);
		}

		refresh_component_table_cache();
	}

	void world_t::begin_component_query()
	{
		_active_component_query_count++;
	}

	void world_t::end_component_query()
	{
		SFG_ASSERT(_active_component_query_count != 0);

		_active_component_query_count--;
	}

	void world_t::refresh_component_table_cache()
	{
		_engine_components.hierarchy_table	  = &get_component_table(type_id_t<component_hierarchy_t>::value);
		_engine_components.guid_table		  = &get_component_table(type_id_t<component_guid_t>::value);
		_engine_components.transform_table	  = &get_component_table(type_id_t<component_transform_t>::value);
		_engine_components.name_table		  = &get_component_table(type_id_t<component_name_t>::value);
		_engine_components.alive_table		  = &get_component_table(type_id_t<component_alive_t>::value);
		_engine_components.prefab_table		  = &get_component_table(type_id_t<component_prefab_reference_t>::value);
		_engine_components.world_script_table = &get_component_table(type_id_t<component_world_script_t>::value);
		_system_components.transform_table	  = &get_component_table(type_id_t<component_system_transform_t>::value);
	}

	const ecs_component_table_t* world_t::find_component_table(sid_t type_id) const
	{
		for (const ecs_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return &table;
		}

		return nullptr;
	}

	ecs_component_table_t* world_t::find_component_table(sid_t type_id)
	{
		for (ecs_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return &table;
		}

		return nullptr;
	}

	const ecs_component_table_t& world_t::get_component_table(sid_t type_id) const
	{
		for (const ecs_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return table;
		}

		SFG_ASSERT(false);
		return _component_tables[0];
	}

	ecs_component_table_t& world_t::get_component_table(sid_t type_id)
	{
		for (ecs_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return table;
		}

		SFG_ASSERT(false);
		return _component_tables[0];
	}

	const vector_t<ecs_component_table_t>& world_t::get_component_tables() const
	{
		return _component_tables;
	}

	const char* world_t::get_entity_name(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_name_t& name = ecs_helpers_t::table_get_as_const<component_name_t>(*_engine_components.name_table, id);
		return name.text;
	}

	const char* world_t::get_text(u32 text_index) const
	{
		if (text_index == ECS_INVALID_INDEX)
			return nullptr;

		SFG_ASSERT(text_index < _text_allocations.size());
		return _text_allocations[text_index].allocated;
	}

	bool world_t::is_alive(entity_id_t id) const
	{
		return ecs_t::table_has(*_engine_components.alive_table, id);
	}

	u32 world_t::allocate_text(const char* text)
	{
		const char* allocated = _text_allocator.allocate(text);
		SFG_ASSERT(allocated != nullptr);
		if (allocated == nullptr)
			return ECS_INVALID_INDEX;

		if (!_text_allocation_free_list.empty())
		{
			const u32 text_index = _text_allocation_free_list.back();
			_text_allocation_free_list.pop_back();
			_text_allocations[text_index].allocated = allocated;
			return text_index;
		}

		const u32 text_index = static_cast<u32>(_text_allocations.size());
		_text_allocations.push_back({.allocated = allocated});
		return text_index;
	}

	void world_t::release_text(u32 text_index)
	{
		if (text_index == ECS_INVALID_INDEX)
			return;

		world_text_allocation_t& allocation = _text_allocations[text_index];
		_text_allocator.deallocate(allocation.allocated);
		allocation.allocated = nullptr;
		_text_allocation_free_list.push_back(text_index);
	}
}
