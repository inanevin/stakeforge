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

#include "core/common/sfg_exports.hpp"
#include "core/gfx/common/gfx_constants.hpp"
#include "core/runtime/world/ecs_defs.hpp"

namespace sfg
{
	template <typename T> struct span_t;
	class world_t;
	class world_render_context_t;
	struct vec2u16_t;
	struct project_settings_t;
	struct world_render_context_config_t;
	struct world_debug_draw_config_t;
	struct world_render_snapshot_t;
	struct world_render_prep_data_t;

	class SFG_API world_renderer_t
	{
	public:
		world_renderer_t() = default;
		virtual ~world_renderer_t();
		world_renderer_t(const world_renderer_t& other)			   = delete;
		world_renderer_t& operator=(const world_renderer_t& other) = delete;

		static inline constexpr u8 SNAPSHOT_SLOT_COUNT = 3;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		virtual void init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config) = 0;
		virtual void uninit()																							   = 0;
		virtual void resize(vec2u16_t size)																				   = 0;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		virtual u8	 produce_snapshot(world_t& world, const project_settings_t& project_settings)																															  = 0;
		virtual void publish_snapshot()																																														  = 0;
		virtual void acquire_snapshot()																																														  = 0;
		virtual void render(world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)															  = 0;
		virtual void draw_selection(gfx_handle_t cmd, const world_render_prep_data_t& prep_data, span_t<const entity_id_t> selected_entities, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const = 0;
		virtual void draw_entity_ids(gfx_handle_t cmd, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout) const											  = 0;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		virtual const world_render_context_t&  get_render_context() const					 = 0;
		virtual gfx_handle_t				   get_depth_texture(u8 frame_index) const		 = 0;
		virtual gpu_index_t					   get_depth_texture_index(u8 frame_index) const = 0;
		virtual const world_render_snapshot_t* get_latest_snapshot() const					 = 0;
		virtual const world_render_snapshot_t& get_render_snapshot() const					 = 0;
		virtual u8							   get_render_snapshot_slot() const				 = 0;
	};
}
