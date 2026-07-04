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

#include "editor_world_controller.hpp"
#include "editor_app.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
#define WORLD_SNAPSHOT_SLOT_COUNT		3
#define WORLD_SNAPSHOT_SLOT_MASK		0x3
#define WORLD_SNAPSHOT_FRESH_FLAG		0x80
#define EDITOR_CAMERA_BASE_MOVE_SPEED	12.0f
#define EDITOR_CAMERA_BOOST_MULTIPLIER	8.0f
#define EDITOR_CAMERA_MOUSE_SENSITIVITY 0.2f

	editor_world_controller_t::world_container_t& editor_world_controller_t::world_container_t::operator=(world_container_t&& other) noexcept
	{
		for (u32 i = 0; i < WORLD_SNAPSHOT_SLOT_COUNT; ++i)
			snapshot_slots[i] = static_cast<world_render_snapshot_t&&>(other.snapshot_slots[i]);

		render_context	= static_cast<world_render_context_t&&>(other.render_context);
		world_resources = static_cast<vector_t<u64>&&>(other.world_resources);
		snapshot_mailbox.store(other.snapshot_mailbox.load(std::memory_order_relaxed), std::memory_order_relaxed);
		handle		  = other.handle;
		producer_slot = other.producer_slot;
		consumer_slot = other.consumer_slot;

		other.snapshot_mailbox.store(0, std::memory_order_relaxed);
		other.world_resources.resize(0);
		other.handle		= {};
		other.producer_slot = 0;
		other.consumer_slot = 0;
		return *this;
	}

	void editor_world_controller_t::init()
	{
		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		reset_camera_input();
		_main_camera_entity = NULL_ENTITY_ID;
	}

	void editor_world_controller_t::uninit()
	{
		destroy_worlds_internal(false);
		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
		reset_camera_input();
		_main_camera_entity = NULL_ENTITY_ID;
	}

	world_handle_t editor_world_controller_t::create_world(vec2u16_t render_resolution)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const world_handle_t handle = _worlds.add();
		world_t&			 world	= _worlds.get(handle);
		world.init();

		world_container_t& container = _world_containers.emplace_back();
		container.handle			 = handle;
		container.producer_slot		 = 0;
		container.consumer_slot		 = 1;
		container.snapshot_mailbox.store(2, std::memory_order_relaxed);
		container.render_context.init(render_resolution);

		for (u32 i = 0; i < 3; i++)
		{
			container.snapshot_slots[i].reserve(8000);
		}

		return handle;
	}

	void editor_world_controller_t::destroy_world(world_handle_t handle)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (auto it = _world_containers.begin(); it != _world_containers.end(); ++it)
		{
			if (it->handle == handle)
			{
				const bool was_main_world = _main_world == handle;

				it->render_context.uninit();
				world_t& world = _worlds.get(handle);
				world.uninit();
				_worlds.remove(handle);
				_world_containers.erase(it);

				if (was_main_world)
				{
					set_main_world({});
					_main_camera_entity = NULL_ENTITY_ID;
				}
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_world_controller_t::destroy_worlds()
	{
		destroy_worlds_internal(true);
	}

	void editor_world_controller_t::destroy_worlds_internal(bool notify_panels)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (world_container_t& container : _world_containers)
		{
			container.render_context.uninit();
			world_t& world = _worlds.get(container.handle);
			world.uninit();
			_worlds.remove(container.handle);
		}
		_world_containers.resize(0);
		_main_camera_entity = NULL_ENTITY_ID;
		reset_camera_input();
		if (notify_panels)
			set_main_world({});
		else
			_main_world = {};
	}

	void editor_world_controller_t::resize_world(world_handle_t handle, vec2u16_t render_resolution)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
			{
				container.render_context.resize(render_resolution);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	bool editor_world_controller_t::render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);

		if (_world_containers.empty())
			return false;

		frame_vector_t<gfx_handle_t> command_buffers;
		command_buffers.reserve(_world_containers.size());

		const f32 interpolation_alpha = calculate_render_alpha();
		for (world_container_t& container : _world_containers)
		{
			const world_render_snapshot_t& snapshot = acquire_render_snapshot(container);
			world_rendering_t::render_world(container.render_context, snapshot, interpolation_alpha, frame_index, global_cbv_index, global_layout);
			command_buffers.push_back(container.render_context.get_command_buffer(frame_index));
		}

		gfx_backend& backend = gfx_backend::get();

		if (!_world_containers.empty())
			backend.queue_signal(queue, &signal, &signal_value, 1);
		return true;
	}

	void editor_world_controller_t::tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps)
	{
		_world_physics_rate = world_physics_rate;

		const i64 now	   = time_t::get_cpu_microseconds();
		const i64 delta_us = now - _previous_time_us;
		_previous_time_us  = now;

		u32		  steps	   = 0;
		const i64 fixed_us = world_tick_rate == 0 ? 0 : 1000000 / static_cast<i64>(world_tick_rate);
		if (fixed_us > 0)
		{
			_accumulator_us += delta_us;

			const f32 dt_seconds = 1.0f / static_cast<f32>(world_tick_rate);
			while (_accumulator_us >= fixed_us && steps < max_sim_steps)
			{
				_accumulator_us -= fixed_us;
				for (const world_container_t& container : _world_containers)
				{
					world_t& world = _worlds.get(container.handle);
					if (container.handle == _main_world)
						tick_editor_camera(dt_seconds);
					world.tick(dt_seconds);
					world.update_world_transforms(true);
				}
				++steps;
			}

			_fixed_step_us.store(fixed_us, std::memory_order_relaxed);
			_last_fixed_step_us.store(now - _accumulator_us, std::memory_order_release);
		}
		else
		{
			_accumulator_us = 0;
			_fixed_step_us.store(0, std::memory_order_relaxed);
			_last_fixed_step_us.store(now, std::memory_order_release);
		}

		for (world_container_t& container : _world_containers)
		{
			world_t& world = _worlds.get(container.handle);
			if (steps == 0)
				world.update_world_transforms(false);
			world_snapshot_producer_t::produce(world, container.snapshot_slots[container.producer_slot]);
			publish_world_snapshot(container);
		}
	}

	void editor_world_controller_t::install_default_world(world_handle_t handle)
	{
		world_t& world = _worlds.get(handle);
		install_editor_camera(world);

		const entity_id_t	environment				  = world.create_entity("environment");
		component_skybox_t& skybox					  = ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(type_id_t<component_skybox_t>::value)->table, environment);
		skybox.skybox_asset							  = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		skybox.exposure								  = 0.25f;
		world_component_table_t& debug_widgets_table  = *world.get_component_table(type_id_t<component_debug_widgets_t>::value);
		const bool				 debug_widgets_exists = ecs_t::table_has(debug_widgets_table.table, environment);
		void*					 debug_widgets		  = ecs_t::table_add(debug_widgets_table.table, environment);
		if (!debug_widgets_exists)
			debug_widgets_table.type_desc.default_init(debug_widgets);

		for (world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
			{
				world.add_resource(resource_type_e::hdr_skybox, DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID);
				world.load_all_used_resources();
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_world_controller_t::set_main_world(world_handle_t handle)
	{
		if (_main_world == handle)
			return;

		editor_app_t::get().get_command_system().clear();
		_main_world = handle;
		notify_main_world_changed();
	}

	void editor_world_controller_t::notify_main_world_changed()
	{
		editor_app_t& app = editor_app_t::get();
		app.get_world_metadata().set_world(_main_world);
		app.get_selection_controller().set_world(_main_world);

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::world))
		{
			editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
			if (_main_world.is_null())
				world_panel->clear_world();
			else
				world_panel->set_world(get_world_render_context(_main_world));
		}

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::entities))
			static_cast<editor_panel_entities_t*>(panel)->refresh_entities();

		if (editor_panel_t* panel = app.find_panel(editor_panel_type_e::inspector))
			static_cast<editor_panel_inspector_t*>(panel)->refresh_from_selection();
	}

	void editor_world_controller_t::reset_input(window_runtime_t& runtime)
	{
		if (_is_looking)
		{
			process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
			process::set_cursor_visible(true);
		}
		reset_camera_input();
	}

	bool editor_world_controller_t::on_window_event(surface_handle_t surface_handle, window_runtime_t& runtime, const window_event_t& ev)
	{
		if (ev.type == window_event_type_e::focus && ev.sub_type == window_event_sub_type_e::release)
		{
			if (_is_looking)
			{
				process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
				process::set_cursor_visible(true);
			}
			reset_camera_input();
			return false;
		}

		editor_panel_t* panel = editor_app_t::get().find_panel_on_surface(editor_panel_type_e::world, surface_handle);
		if (panel == nullptr || !panel->is_inited())
			return false;

		editor_panel_world_t* world_panel = static_cast<editor_panel_world_t*>(panel);
		const vec4f_t		  bounds	  = world_panel->get_world_view_bounds();
		const vec2f_t		  mouse		  = {static_cast<f32>(runtime.mouse_position.x), static_cast<f32>(runtime.mouse_position.y)};
		const bool			  inside	  = mouse.x >= bounds.x && mouse.y >= bounds.y && mouse.x <= bounds.x + bounds.z && mouse.y <= bounds.y + bounds.w;

		switch (ev.type)
		{
		case window_event_type_e::mouse:
			if (ev.sub_type == window_event_sub_type_e::press)
			{
				if (!inside)
				{
					if (_is_looking)
					{
						process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
						process::set_cursor_visible(true);
					}
					reset_camera_input();
					return false;
				}

				_world_panel_focused = true;
				if (ev.button == static_cast<u16>(input_code::mouse_right))
				{
					_is_looking = true;
					process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::pointer);
					process::set_cursor_visible(false);
				}
				return true;
			}

			if (ev.sub_type == window_event_sub_type_e::release && ev.button == static_cast<u16>(input_code::mouse_right) && _is_looking)
			{
				_is_looking	 = false;
				_mouse_delta = vec2f_t::zero;
				process::set_cursor_confinement(runtime.window_handle, window_cursor_confinement_e::none);
				process::set_cursor_visible(true);
				return true;
			}
			break;
		case window_event_type_e::delta:
			if (_is_looking)
			{
				_mouse_delta.x += static_cast<f32>(ev.value.x);
				_mouse_delta.y += static_cast<f32>(ev.value.y);
				return true;
			}
			break;
		case window_event_type_e::key:
			if (!_world_panel_focused)
				return false;

			if (ev.button == static_cast<u16>(input_code::key_w) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.z += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_w) && ev.sub_type == window_event_sub_type_e::release && _direction_input.z > 0.1f)
				_direction_input.z -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.z -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::release && _direction_input.z < -0.1f)
				_direction_input.z += 1.0f;

			if (ev.button == static_cast<u16>(input_code::key_d) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.x += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_d) && ev.sub_type == window_event_sub_type_e::release && _direction_input.x > 0.1f)
				_direction_input.x -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_a) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.x -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_a) && ev.sub_type == window_event_sub_type_e::release && _direction_input.x < -0.1f)
				_direction_input.x += 1.0f;

			if (ev.button == static_cast<u16>(input_code::key_e) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.y += 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_e) && ev.sub_type == window_event_sub_type_e::release && _direction_input.y > 0.1f)
				_direction_input.y -= 1.0f;
			if (ev.button == static_cast<u16>(input_code::key_q) && ev.sub_type == window_event_sub_type_e::press)
				_direction_input.y -= 1.0f;
			else if (ev.button == static_cast<u16>(input_code::key_q) && ev.sub_type == window_event_sub_type_e::release && _direction_input.y < -0.1f)
				_direction_input.y += 1.0f;

			if ((ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift)) && ev.sub_type == window_event_sub_type_e::press)
				_current_move_speed = EDITOR_CAMERA_BASE_MOVE_SPEED * EDITOR_CAMERA_BOOST_MULTIPLIER;
			else if ((ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift)) && ev.sub_type == window_event_sub_type_e::release)
				_current_move_speed = EDITOR_CAMERA_BASE_MOVE_SPEED;

			if (ev.button == static_cast<u16>(input_code::key_a) || ev.button == static_cast<u16>(input_code::key_d) || ev.button == static_cast<u16>(input_code::key_w) || ev.button == static_cast<u16>(input_code::key_s) ||
				ev.button == static_cast<u16>(input_code::key_q) || ev.button == static_cast<u16>(input_code::key_e) || ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift))
				return true;
			break;
		default:
			break;
		}

		return false;
	}

	const world_render_context_t& editor_world_controller_t::get_world_render_context(world_handle_t handle) const
	{
		for (const world_container_t& container : _world_containers)
		{
			if (container.handle == handle)
				return container.render_context;
		}

		SFG_ASSERT(false);
		return _world_containers.front().render_context;
	}

	void editor_world_controller_t::publish_world_snapshot(world_container_t& container)
	{
		const u8 prev			= container.snapshot_mailbox.exchange(container.producer_slot | WORLD_SNAPSHOT_FRESH_FLAG, std::memory_order_release);
		container.producer_slot = static_cast<u8>((prev & WORLD_SNAPSHOT_SLOT_MASK) % WORLD_SNAPSHOT_SLOT_COUNT);
	}

	const world_render_snapshot_t& editor_world_controller_t::acquire_render_snapshot(world_container_t& container)
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 cur = container.snapshot_mailbox.load(std::memory_order_acquire);
		while (cur & WORLD_SNAPSHOT_FRESH_FLAG)
		{
			if (container.snapshot_mailbox.compare_exchange_weak(cur, container.consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				container.consumer_slot = static_cast<u8>((cur & WORLD_SNAPSHOT_SLOT_MASK) % WORLD_SNAPSHOT_SLOT_COUNT);
				break;
			}
		}
		return container.snapshot_slots[container.consumer_slot];
	}

	f32 editor_world_controller_t::calculate_render_alpha() const
	{
		const i64 fixed_us = _fixed_step_us.load(std::memory_order_relaxed);
		if (fixed_us <= 0)
			return 0.0f;

		const i64 last_fixed_step_us = _last_fixed_step_us.load(std::memory_order_acquire);
		const i64 now				 = time_t::get_cpu_microseconds();
		const f32 alpha				 = static_cast<f32>(static_cast<double>(now - last_fixed_step_us) / static_cast<double>(fixed_us));
		if (alpha < 0.0f)
			return 0.0f;
		if (alpha > 1.0f)
			return 1.0f;
		return alpha;
	}

	void editor_world_controller_t::install_editor_camera(world_t& world)
	{
		const entity_id_t	camera_entity = world.create_entity("editor camera");
		component_camera_t& camera		  = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value)->table, camera_entity);
		camera.priority					  = -1;
		ecs_t::table_add(world.get_component_table(type_id_t<component_no_serialize_t>::value)->table, camera_entity);
		ecs_t::table_add(world.get_component_table(type_id_t<component_render_object_t>::value)->table, camera_entity);
		_main_camera_entity	  = camera_entity;
		const vec3f_t euler	  = quat_t::to_euler(world.get_entity_rot_local(camera_entity));
		_camera_pitch_degrees = euler.x;
		_camera_yaw_degrees	  = euler.y;
	}

	void editor_world_controller_t::reset_camera_input()
	{
		_direction_input	 = vec3f_t::zero;
		_mouse_delta		 = vec2f_t::zero;
		_current_move_speed	 = EDITOR_CAMERA_BASE_MOVE_SPEED;
		_world_panel_focused = false;
		_is_looking			 = false;
	}

	void editor_world_controller_t::tick_editor_camera(f32 dt_seconds)
	{
		if (_main_camera_entity == NULL_ENTITY_ID || !_world_panel_focused)
			return;

		world_t& world = _worlds.get(_main_world);

		_camera_yaw_degrees -= _mouse_delta.x * EDITOR_CAMERA_MOUSE_SENSITIVITY;
		_camera_pitch_degrees -= _mouse_delta.y * EDITOR_CAMERA_MOUSE_SENSITIVITY;
		_camera_pitch_degrees = math::clamp(_camera_pitch_degrees, -89.0f, 89.0f);
		_mouse_delta		  = vec2f_t::zero;

		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		world.set_entity_rot_local(_main_camera_entity, rotation);

		vec3f_t move_dir = rotation.get_forward() * _direction_input.z + rotation.get_right() * _direction_input.x + vec3f_t::up * _direction_input.y;
		if (move_dir.is_zero())
			return;

		move_dir.normalize();
		const vec3f_t position = world.get_entity_pos_local(_main_camera_entity) + move_dir * (_current_move_speed * dt_seconds);
		world.set_entity_pos_local(_main_camera_entity, position);
	}
}
