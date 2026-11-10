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
#include "core/data/span.hpp"
#include "core/gfx/common/gfx_constants.hpp"

namespace sfg
{
	namespace ui
	{
		class ui_renderer_t;
	}
	struct world_canvas_draw_snapshot_t;
	class stock_world_renderer_context_t;
	struct stock_world_renderer_prep_data_t;
	struct world_renderer_view_t;
	struct stock_world_renderer_reflection_allocation_t;
	struct stock_world_renderer_snapshot_t;
	struct stock_world_renderer_draw_packet_t;
	struct stock_world_renderer_draw_state_t;
	struct stock_world_renderer_queue_item_t;
	struct stock_world_renderer_material_t;

	struct stock_world_renderer_clustered_lighting_data_t
	{
		gfx_handle_t command_buffer				  = {};
		gfx_handle_t cluster_buffer				  = {};
		gfx_handle_t cluster_light_indices_buffer = {};
		gfx_handle_t shader						  = {};
		gpu_index_t	 lighting_data_index		  = NULL_GPU_INDEX;
		gpu_index_t	 global_cbv_index			  = NULL_GPU_INDEX;
	};

	struct stock_world_renderer_clustered_lighting_view_t
	{
		gpu_index_t view_data_index = NULL_GPU_INDEX;
		u32			cluster_count_x = 0;
		u32			cluster_count_y = 0;
		u32			cluster_count_z = 0;
	};

	class SFG_API stock_world_renderer_passes_t final
	{
	public:
		static void draw_queue(
			gfx_handle_t cmd, const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, span_t<const stock_world_renderer_queue_item_t> draws, u32 required_pass_mask, u32 initial_variant_flags, u8 frame_index);
		static void draw_packets(gfx_handle_t cmd, span_t<const stock_world_renderer_draw_packet_t> packets, span_t<const stock_world_renderer_material_t> materials, u8 frame_index, stock_world_renderer_draw_state_t& state);

		static void render_depth_prepass(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_gbuffer(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_shadows(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_probe(gfx_handle_t										 command_buffer,
								 const stock_world_renderer_context_t&				 ctx,
								 const stock_world_renderer_snapshot_t&				 snapshot,
								 const stock_world_renderer_prep_data_t&			 prep_data,
								 const stock_world_renderer_reflection_allocation_t& allocation,
								 const u16*											 cull_view_indices,
								 bool												 skybox_only,
								 u8													 frame_index,
								 gpu_index_t										 global_cbv_index,
								 gfx_handle_t										 global_layout);
		static void render_clustered_lighting(const stock_world_renderer_clustered_lighting_data_t& data, span_t<const stock_world_renderer_clustered_lighting_view_t> views);
		static void render_prefilter_diffuse_sh(const stock_world_renderer_context_t& ctx, const stock_world_renderer_reflection_allocation_t& allocation, u8 frame_index, gpu_index_t global_cbv_index);
		static void render_ssao(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index);
		static void render_lighting(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_canvas(gfx_handle_t cmd, const stock_world_renderer_context_t& ctx, const world_canvas_draw_snapshot_t& canvas, ui::ui_renderer_t& renderer, gfx_handle_t target, u8 frame_index);
		static void render_forward(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_bloom(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index);
		static gfx_handle_t begin_post_process(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void			end_post_process(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index);
		static void			render_post_process(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void			render_finalize(const stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
	};
}
