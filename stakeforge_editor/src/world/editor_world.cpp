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
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>

namespace sfg
{
#define EDITOR_WORLD_SNAPSHOT_SLOT_COUNT 3
#define EDITOR_WORLD_SNAPSHOT_SLOT_MASK	 0x3
#define EDITOR_WORLD_SNAPSHOT_FRESH_FLAG 0x80

	void editor_world_t::init(const world_init_config_t& init_config, editor_world_handle_t handle)
	{
		_world.init(init_config);
		_edit_context.init();
		_edit_context.set_world(handle);
		_producer_slot = 0;
		_consumer_slot = 1;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
		_render_resolution = init_config.render_resolution;
		_renderer.init(init_config.render_resolution, {.data = _snapshot_slots, .size = EDITOR_WORLD_SNAPSHOT_SLOT_COUNT});

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
			_snapshot_slots[i].reserve(8000);
	}

	void editor_world_t::uninit()
	{
		uninstall_camera();
		_renderer.uninit({.data = _snapshot_slots, .size = EDITOR_WORLD_SNAPSHOT_SLOT_COUNT});
		_edit_context.uninit();
		_world.unload_all_used_resources();
		_world.uninit();
		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_render_resolution = vec2u16_t::zero;
		_producer_slot	   = 0;
		_consumer_slot	   = 0;
	}

	void editor_world_t::resize(vec2u16_t render_resolution)
	{
		_render_resolution = render_resolution;
		_renderer.resize(render_resolution);
	}

	void editor_world_t::install_camera(editor_world_camera_type_e type)
	{
		uninstall_camera();

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

		_camera->uninit(_world);
		delete _camera;
		_camera = nullptr;
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

	void editor_world_t::tick(f32 dt_seconds)
	{
		_world.tick(dt_seconds);
		_world.update_world_transforms(true);
	}

	void editor_world_t::update_world_transforms(bool advance_interpolation)
	{
		_world.update_world_transforms(advance_interpolation);
	}

	void editor_world_t::produce_snapshot()
	{
		world_render_snapshot_t& snapshot = _snapshot_slots[_producer_slot];
		world_snapshot_producer_t::produce(_world, snapshot);

		editor_world_snapshot_data_t&	data			= *static_cast<editor_world_snapshot_data_t*>(snapshot.user_data);
		const span_t<const entity_id_t> selected		= _edit_context.get_selected_entities();
		const ecs_component_table_t&	hierarchy_table = _world.get_component_table(type_id_t<component_hierarchy_t>::value);
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
}
