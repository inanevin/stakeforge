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
#include "core/gfx/backend/backend.hpp"
#include "core/gfx/common/commands.hpp"
#include "core/gfx/util/gfx_util.hpp"
#include "core/io/assert.hpp"
#include "core/runtime/engine/engine_threads.hpp"
#include "core/runtime/render/world_rendering.hpp"
#include "core/runtime/render/world_draw_common.hpp"
#include "core/runtime/resources/shader_types.hpp"
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

	void stock_world_renderer_t::draw_selection(gfx_handle_t cmd, const world_render_prep_data_t& prep_data, span_t<const entity_id_t> selected_entities, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		gfx_backend&				   backend		  = gfx_backend::get();
		const world_render_snapshot_t& snapshot		  = _snapshot_slots[_consumer_slot];
		const gpu_index_t			   rp_constants[] = {
			_render_context.get_view_render_pass_data_index(frame_index),
			_render_context.get_entity_buffer_index(frame_index),
			_render_context.get_bone_buffer_index(frame_index),
		};

		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = std::size(rp_constants), .param_index = 0});

		const world_render_prep_view_t&				  prep_view		 = prep_data.views[0];
		const span_t<const world_render_queue_item_t> visible_draws	 = prep_data.get_visible_queue(prep_view);
		frame_vector_t<world_render_queue_item_t>	  selected_draws = {};
		selected_draws.reserve(visible_draws.size);

		for (const world_render_queue_item_t& item : visible_draws)
		{
			const world_renderable_t&	 renderable = snapshot.renderables[item.renderable_index];
			const world_render_entity_t& entity		= snapshot.entities[renderable.entity_index];

			if (std::find(selected_entities.begin(), selected_entities.end(), entity.entity_id) != selected_entities.end())
				selected_draws.push_back(item);
		}

		world_rendering_t::draw_queue(cmd, _render_context, snapshot, {.data = selected_draws.data(), .size = selected_draws.size()}, 0, shader_variant_flags_selection_outline, frame_index);
	}

	void stock_world_renderer_t::draw_entity_ids(gfx_handle_t cmd, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		gfx_backend&				   backend		  = gfx_backend::get();
		const world_render_snapshot_t& snapshot		  = _snapshot_slots[_consumer_slot];
		const gpu_index_t			   rp_constants[] = {
			_render_context.get_view_render_pass_data_index(frame_index),
			_render_context.get_entity_buffer_index(frame_index),
			_render_context.get_bone_buffer_index(frame_index),
		};

		backend.cmd_bind_layout(cmd, {.layout = global_layout});
		backend.cmd_bind_constants(cmd, {.data = &global_cbv_index, .offset = constant_global0, .count = 1, .param_index = 0});
		backend.cmd_bind_constants(cmd, {.data = rp_constants, .offset = constant_rp0, .count = std::size(rp_constants), .param_index = 0});

		const world_render_prep_view_t& prep_view = prep_data.views[0];

		world_rendering_t::draw_queue(cmd, _render_context, snapshot, prep_data.get_visible_queue(prep_view), world_pass_flags_id, shader_variant_flags_id_write, frame_index);
	}
}
