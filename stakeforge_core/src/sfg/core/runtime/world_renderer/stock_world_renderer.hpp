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
#include "world_renderer_view.hpp"
#include "stock_world_renderer_context.hpp"
#include "stock_world_renderer_snapshot.hpp"

namespace sfg
{
	class SFG_API stock_world_renderer_t : public world_renderer_t
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
		void build_render_view(const world_renderer_camera_t& camera, const vec2u16_t& resolution, f32 interpolation_alpha, world_renderer_view_t& out_view) const override;
		void render_scene(f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) override;
		void render_post_process(u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) override;
		void render_finalize(u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) override;
		void draw_selection(gfx_handle_t cmd, span_t<const entity_id_t> selected_entities, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const override;
		void draw_entity_ids(gfx_handle_t cmd, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const override;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		vec2u16_t get_size() const override
		{
			return _render_context.get_size();
		}

		const world_renderer_view_t& get_render_view(u8 frame_index) const override
		{
			return _render_views[frame_index];
		}

		gpu_index_t get_world_texture_final(u8 frame_index) const override
		{
			return _render_context.get_world_texture_index(frame_index);
		}

		gpu_index_t get_world_texture_unlit(u8 frame_index) const override
		{
			return _render_context.get_gbuffer_albedo_index(frame_index);
		}

		gfx_handle_t get_depth_texture(u8 frame_index) const override
		{
			return _render_context.get_depth_texture(frame_index);
		}

		gpu_index_t get_depth_texture_index(u8 frame_index) const override
		{
			return _render_context.get_depth_texture_index(frame_index);
		}

		u8 get_render_snapshot_slot() const override
		{
			return _consumer_slot;
		}

	protected:
		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		stock_world_renderer_context_t& get_render_context()
		{
			return _render_context;
		}
		const stock_world_renderer_snapshot_t& get_render_snapshot() const
		{
			return _snapshot_slots[_consumer_slot];
		}

	private:
		frame_vector_t<stock_world_renderer_queue_item_t> _visible_draws					   = {};
		world_renderer_view_t							  _render_views[BACK_BUFFER_COUNT]	   = {};
		stock_world_renderer_context_t					  _render_context					   = {};
		stock_world_renderer_snapshot_t					  _snapshot_slots[SNAPSHOT_SLOT_COUNT] = {};
		atomic_t<u8>									  _snapshot_mailbox					   = {};
		u8												  _producer_slot					   = 0;
		u8												  _consumer_slot					   = 0;
	};
}
