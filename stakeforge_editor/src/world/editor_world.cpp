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
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>

namespace sfg
{
#define EDITOR_WORLD_SNAPSHOT_SLOT_COUNT	   3
#define EDITOR_WORLD_SNAPSHOT_SLOT_MASK		   0x3
#define EDITOR_WORLD_SNAPSHOT_FRESH_FLAG	   0x80
#define EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT 32

	void editor_world_t::init(const world_init_config_t& init_config, editor_world_handle_t handle)
	{
		_world.init(init_config);
		_edit_context.init();
		_edit_context.set_world(handle);
		_producer_slot = 0;
		_consumer_slot = 1;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
		_pick_result.store(0, std::memory_order_relaxed);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_next_pick_request_id		 = 0;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;

		_render_resolution = init_config.render_resolution;
		_render_context.init(init_config.render_resolution);

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
		{
			_snapshot_slots[i].reserve(8000);
			editor_world_snapshot_data_t* data = new editor_world_snapshot_data_t();
			data->selected_entities.reserve(256);
			_snapshot_slots[i].user_data = data;
		}
	}

	void editor_world_t::uninit()
	{
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

		_render_resolution = vec2u16_t::zero;
		_producer_slot	   = 0;
		_consumer_slot	   = 0;
	}

	void editor_world_t::resize(vec2u16_t render_resolution)
	{
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

		const span_t<const entity_id_t> selected = _edit_context.get_selected_entities();
		frame_vector_t<entity_id_t>		selection;
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

	void editor_world_t::tick(f32 dt_seconds)
	{
		consume_entity_pick_result();
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

		data.pick_request		 = _pending_pick_request;
		data.gizmo				 = {};
		const entity_id_t anchor = _edit_context.get_entity_anchor();
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

	void editor_world_t::render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
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

		world_rendering_t::render_world(_render_context.get_world_render_context(), snapshot, interpolation_alpha, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_outlines(_render_context, snapshot, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_object_ids(_render_context, snapshot, frame_index);
		editor_world_rendering_t::blit_world_texture(_render_context, snapshot, interpolation_alpha, frame_index);
		_object_id_readback_valid[frame_index] = true;
	}
}
