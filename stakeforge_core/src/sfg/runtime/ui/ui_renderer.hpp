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

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg::ui
{
	struct vg_draw_snapshot_t;

	struct ui_renderer_config_t
	{
		u32 vertex_buffer_max_bytes = 1u << 20;
		u32 index_buffer_max_bytes	= 1u << 20;
	};

	class ui_renderer_t final
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
		void render(gfx_handle_t cmd, const vg_draw_snapshot_t* snap, u8 frame_index, vec2u16_t fb_size);

	private:
		struct per_frame_data_t
		{
			gfx_handle_t vertex_buffer	   = {};
			gfx_handle_t index_buffer	   = {};
			gfx_handle_t projection_buffer = {};
			u8*			 mapped_vtx		   = nullptr;
			u8*			 mapped_idx		   = nullptr;
			u8*			 mapped_projection = nullptr;
			gpu_index_t	 projection_index  = 0;
		};

	private:
		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};
		gfx_handle_t	 _sdf_params			 = {};
		gpu_index_t		 _sdf_params_index		 = 0;
		u32				 _vtx_capacity			 = 0;
		u32				 _idx_capacity			 = 0;
	};
}
