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

#include "stock_world_renderer.hpp"
#include "core/io/assert.hpp"
#include "core/runtime/engine/engine_threads.hpp"
#include "core/runtime/render/world_rendering.hpp"
#include "core/runtime/world/world_snapshot_producer.hpp"

namespace sfg
{
#define WORLD_RENDERER_SNAPSHOT_SLOT_MASK  0x3
#define WORLD_RENDERER_SNAPSHOT_FRESH_FLAG 0x80

	void stock_world_renderer_t::init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config)
	{
		_render_context.init(config, debug_draw_config);

		for (world_render_snapshot_t& snapshot : _snapshot_slots)
			snapshot.reserve();

		_producer_slot		  = 0;
		_consumer_slot		  = 1;
		_latest_snapshot_slot = UINT8_MAX;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
	}

	void stock_world_renderer_t::uninit()
	{
		_render_context.uninit();

		for (world_render_snapshot_t& snapshot : _snapshot_slots)
			snapshot = {};

		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_producer_slot		  = 0;
		_consumer_slot		  = 0;
		_latest_snapshot_slot = UINT8_MAX;
	}

	void stock_world_renderer_t::resize(vec2u16_t size)
	{
		_render_context.resize(size);
	}

	u8 stock_world_renderer_t::produce_snapshot(world_t& world, const project_settings_t& project_settings)
	{
		world_snapshot_producer_t::produce(world, _snapshot_slots[_producer_slot], project_settings, _render_context);
		_latest_snapshot_slot = _producer_slot;
		return _producer_slot;
	}

	void stock_world_renderer_t::publish_snapshot()
	{
		const u8 previous = _snapshot_mailbox.exchange(_producer_slot | WORLD_RENDERER_SNAPSHOT_FRESH_FLAG, std::memory_order_acq_rel);
		_producer_slot	  = previous & WORLD_RENDERER_SNAPSHOT_SLOT_MASK;
	}

	void stock_world_renderer_t::acquire_snapshot()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 current = _snapshot_mailbox.load(std::memory_order_acquire);

		while (current & WORLD_RENDERER_SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(current, _consumer_slot, std::memory_order_acq_rel, std::memory_order_acquire))
			{
				_consumer_slot = current & WORLD_RENDERER_SNAPSHOT_SLOT_MASK;
				break;
			}
		}
	}

	void stock_world_renderer_t::render(world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		prep_data.reserve();
		world_rendering_t::render_world(_render_context, _snapshot_slots[_consumer_slot], prep_data, interpolation_alpha, frame_index, global_cbv_index, global_layout);
	}
}
