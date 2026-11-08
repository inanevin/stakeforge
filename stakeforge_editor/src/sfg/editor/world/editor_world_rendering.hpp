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

#include "editor/editor_exports.hpp"

#include <sfg/core/gfx/common/gfx_constants.hpp>

namespace sfg
{
	class editor_world_render_context_t;
	class world_render_context_t;
	class world_renderer_t;
	struct editor_world_snapshot_data_t;
	struct world_render_snapshot_t;
	struct world_render_prep_data_t;

	class SFG_EDITOR_API editor_world_rendering_t final
	{
	public:
		static void render_world(const editor_world_render_context_t& ctx,
								 const world_renderer_t&			  world_renderer,
								 const editor_world_snapshot_data_t&  snapshot_data,
								 world_render_prep_data_t&			  prep_data,
								 f32								  interpolation_alpha,
								 u8									  frame_index,
								 gpu_index_t						  global_cbv_index,
								 gfx_handle_t						  global_layout);

	private:
		static void render_outlines(const editor_world_render_context_t& ctx,
									const world_renderer_t&				 world_renderer,
									const editor_world_snapshot_data_t&	 snapshot_data,
									const world_render_prep_data_t&		 prep_data,
									u8									 frame_index,
									gpu_index_t							 global_cbv_index,
									gfx_handle_t						 global_layout);
		static void render_object_ids(const editor_world_render_context_t& ctx,
									  const world_renderer_t&			   world_renderer,
									  const editor_world_snapshot_data_t&  snapshot_data,
									  const world_render_prep_data_t&	   prep_data,
									  u8								   frame_index,
									  gpu_index_t						   global_cbv_index,
									  gfx_handle_t						   global_layout);
		static void prep_icons(const editor_world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const editor_world_snapshot_data_t& snapshot_data, f32 interpolation_alpha, u8 frame_index);
		static void draw_icons(const editor_world_render_context_t& ctx, gfx_handle_t shader, gpu_index_t depth_texture_index, u32 icon_count, u8 frame_index);
		static void blit_world_texture(const editor_world_render_context_t& ctx,
									   const world_render_context_t&		world_ctx,
									   gpu_index_t							source_texture_index,
									   gpu_index_t							depth_texture_index,
									   const world_render_snapshot_t&		snapshot,
									   const editor_world_snapshot_data_t&	snapshot_data,
									   world_render_prep_data_t&			prep_data,
									   f32									interpolation_alpha,
									   u8									frame_index);
	};
}
