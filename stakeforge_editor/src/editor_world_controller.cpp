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
#include <sfg/data/frame_vector.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>

namespace sfg
{
#define WORLD_SNAPSHOT_SLOT_COUNT 3
#define WORLD_SNAPSHOT_SLOT_MASK  0x3
#define WORLD_SNAPSHOT_FRESH_FLAG 0x80

	editor_world_controller_t::world_container_t& editor_world_controller_t::world_container_t::operator=(world_container_t&& other) noexcept
	{
		for (u32 i = 0; i < WORLD_SNAPSHOT_SLOT_COUNT; ++i)
			snapshot_slots[i] = static_cast<world_render_snapshot_t&&>(other.snapshot_slots[i]);

		render_context = static_cast<world_render_context_t&&>(other.render_context);
		snapshot_mailbox.store(other.snapshot_mailbox.load(std::memory_order_relaxed), std::memory_order_relaxed);
		handle		  = other.handle;
		producer_slot = other.producer_slot;
		consumer_slot = other.consumer_slot;

		other.snapshot_mailbox.store(0, std::memory_order_relaxed);
		other.handle		= {};
		other.producer_slot = 0;
		other.consumer_slot = 0;
		return *this;
	}

	void editor_world_controller_t::init(engine_runtime_t& runtime)
	{
		_runtime		  = &runtime;
		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(_previous_time_us, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
	}

	void editor_world_controller_t::uninit()
	{
		destroy_worlds();
		_runtime		  = nullptr;
		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_last_fixed_step_us.store(0, std::memory_order_relaxed);
		_fixed_step_us.store(0, std::memory_order_relaxed);
	}

	world_handle_t editor_world_controller_t::create_world(vec2u16_t render_resolution)
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		const world_handle_t handle = _runtime->create_world();

		world_container_t& container = _worlds.emplace_back();
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
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (auto it = _worlds.begin(); it != _worlds.end(); ++it)
		{
			if (it->handle == handle)
			{
				it->render_context.uninit();
				_runtime->destroy_world(handle);
				_worlds.erase(it);
				if (_main_world == handle)
					_main_world = {};
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_world_controller_t::destroy_worlds()
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (world_container_t& container : _worlds)
		{
			container.render_context.uninit();
			_runtime->destroy_world(container.handle);
		}
		_worlds.resize(0);
		_main_world = {};
	}

	void editor_world_controller_t::resize_world(world_handle_t handle, vec2u16_t render_resolution)
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		for (world_container_t& container : _worlds)
		{
			if (container.handle == handle)
			{
				container.render_context.resize(render_resolution);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	bool editor_world_controller_t::render_worlds(gfx_queue_handle queue, gfx_semaphore_handle signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_bind_layout_handle global_layout)
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);

		if (_worlds.empty())
			return false;

		frame_vector_t<gfx_command_buffer_handle> command_buffers;
		command_buffers.reserve(_worlds.size());

		const f32 interpolation_alpha = calculate_render_alpha();
		for (world_container_t& container : _worlds)
		{
			const world_render_snapshot_t& snapshot = acquire_render_snapshot(container);
			world_rendering_t::render_world(container.render_context, snapshot, interpolation_alpha, frame_index, global_cbv_index, global_layout);
			command_buffers.push_back(container.render_context.get_command_buffer(frame_index));
		}

		gfx_backend& backend = gfx_backend::get();

		if (!_worlds.empty())
			backend.queue_signal(queue, &signal, &signal_value, 1);
		return true;
	}

	void editor_world_controller_t::tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps)
	{
		SFG_ASSERT(_runtime != nullptr);
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
				for (const world_container_t& container : _worlds)
				{
					world_t& world = _runtime->get_world(container.handle);
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

		for (world_container_t& container : _worlds)
		{
			world_t& world = _runtime->get_world(container.handle);
			if (steps == 0)
				world.update_world_transforms(false);
			world_snapshot_producer_t::produce(world, container.snapshot_slots[container.producer_slot]);
			publish_world_snapshot(container);
		}
	}

	void editor_world_controller_t::install_default_world(world_handle_t handle)
	{
		SFG_ASSERT(_runtime != nullptr);

		world_t& world = _runtime->get_world(handle);
		install_editor_camera(world);

		const entity_id_t environment = world.create_entity("environment");
		ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(component_skybox_t::TYPE_ID)->table, environment);
	}

	void editor_world_controller_t::set_main_world(world_handle_t handle)
	{
		_main_world = handle;
	}

	const world_render_context_t& editor_world_controller_t::get_world_render_context(world_handle_t handle) const
	{
		for (const world_container_t& container : _worlds)
		{
			if (container.handle == handle)
				return container.render_context;
		}

		SFG_ASSERT(false);
		return _worlds.front().render_context;
	}

	world_handle_t editor_world_controller_t::get_main_world() const
	{
		return _main_world;
	}

	f32 editor_world_controller_t::get_alpha() const
	{
		return calculate_render_alpha();
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
		component_camera_t& camera		  = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(component_camera_t::TYPE_ID)->table, camera_entity);
		camera.priority					  = -1;
		ecs_t::table_add(world.get_component_table(component_no_serialize_t::TYPE_ID)->table, camera_entity);
	}
}
