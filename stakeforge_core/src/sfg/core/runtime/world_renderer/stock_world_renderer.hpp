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

#pragma once

#include "world_renderer.hpp"
#include "core/data/atomic.hpp"
#include "core/runtime/render/world_render_context.hpp"
#include "core/runtime/render/world_render_snapshot.hpp"

namespace sfg
{
	class SFG_API stock_world_renderer_t final : public world_renderer_t
	{
	public:
		stock_world_renderer_t()											   = default;
		~stock_world_renderer_t() override									   = default;
		stock_world_renderer_t(const stock_world_renderer_t& other)			   = delete;
		stock_world_renderer_t& operator=(const stock_world_renderer_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config) override;
		void uninit() override;
		void resize(vec2u16_t size) override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		u8	 produce_snapshot(world_t& world, const project_settings_t& project_settings) override;
		void publish_snapshot() override;
		void acquire_snapshot() override;
		void render(world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) override;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		const world_render_context_t& get_render_context() const override
		{
			return _render_context;
		}

		const world_render_snapshot_t* get_latest_snapshot() const override
		{
			return _latest_snapshot_slot == UINT8_MAX ? nullptr : &_snapshot_slots[_latest_snapshot_slot];
		}

		const world_render_snapshot_t& get_render_snapshot() const override
		{
			return _snapshot_slots[_consumer_slot];
		}

		u8 get_render_snapshot_slot() const override
		{
			return _consumer_slot;
		}

	private:
		world_render_context_t	_render_context						 = {};
		world_render_snapshot_t _snapshot_slots[SNAPSHOT_SLOT_COUNT] = {};
		atomic_t<u8>			_snapshot_mailbox					 = {};
		u8						_producer_slot						 = 0;
		u8						_consumer_slot						 = 0;
		u8						_latest_snapshot_slot				 = UINT8_MAX;
	};
}
