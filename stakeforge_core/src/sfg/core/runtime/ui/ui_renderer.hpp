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

#include "core/common/size_definitions.hpp"
#include "core/gfx/common/gfx_constants.hpp"
#include "core/math/vec2u16.hpp"

namespace sfg::ui
{
	struct vg_draw_snapshot_t;
	struct vg_draw_batch_t;
	struct ui_canvas_data_t;

	struct ui_world_render_data_t
	{
		const ui_canvas_data_t* canvas_data		  = nullptr;
		u32						canvas_data_count = 0;
		gpu_index_t				view_index		  = NULL_GPU_INDEX;
		gpu_index_t				entity_index	  = NULL_GPU_INDEX;
	};

	struct ui_renderer_config_t
	{
		u32 vertex_buffer_max_bytes = 1u << 20;
		u32 index_buffer_max_bytes	= 1u << 20;
		u32 world_canvas_max		= 0;
	};

	class SFG_API ui_renderer_t final
	{
	public:
		ui_renderer_t()								   = default;
		~ui_renderer_t()							   = default;
		ui_renderer_t(const ui_renderer_t&)			   = delete;
		ui_renderer_t& operator=(const ui_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const ui_renderer_config_t& cfg = {});
		void uninit();
		void render(gfx_handle_t cmd, const vg_draw_snapshot_t* snap, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data = {});
		bool upload(const vg_draw_snapshot_t& snap, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data = {});
		void draw(gfx_handle_t cmd, const vg_draw_batch_t* batches, u32 batch_count, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data = {});

	private:
		struct per_frame_data_t
		{
			u8*			 mapped_canvas_data = nullptr;
			u8*			 mapped_vtx			= nullptr;
			u8*			 mapped_idx			= nullptr;
			u8*			 mapped_projection	= nullptr;
			u8*			 mapped_parameters	= nullptr;
			gfx_handle_t canvas_data_buffer = {};
			gfx_handle_t vertex_buffer		= {};
			gfx_handle_t index_buffer		= {};
			gfx_handle_t projection_buffer	= {};
			gfx_handle_t parameter_buffer	= {};
			gpu_index_t	 canvas_data_index	= NULL_GPU_INDEX;
			gpu_index_t	 projection_index	= NULL_GPU_INDEX;
			gpu_index_t	 parameter_index	= NULL_GPU_INDEX;
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		u32				 _canvas_data_capacity	 = 0;
		u32				 _vtx_capacity			 = 0;
		u32				 _idx_capacity			 = 0;
		u32				 _parameter_capacity	 = 0;
	};
}
