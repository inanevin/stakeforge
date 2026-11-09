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
	struct world_renderer_view_t;
	class stock_world_renderer_context_t;
	class stock_world_renderer_reflection_context_t;
	struct stock_world_renderer_prep_data_t;
	struct stock_world_renderer_reflection_allocation_t;
	struct stock_world_renderer_reflection_probe_t;
	struct stock_world_renderer_snapshot_t;
	struct stock_world_renderer_queue_item_t;
	struct stock_world_renderer_draw_packet_t;

	class SFG_API stock_world_renderer_util_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		static void prep_entity_buffer(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index);
		static void prep_bone_buffer(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index);
		static void prep_shadows(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, stock_world_renderer_prep_data_t& prep_data, const world_renderer_view_t& main_camera_view, f32 interpolation_alpha);
		static void prep_light_buffer(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, u32 (&light_counts)[4]);
		static void prep_probe(stock_world_renderer_reflection_context_t&	  reflection_context,
							   stock_world_renderer_reflection_allocation_t&  allocation,
							   const stock_world_renderer_reflection_probe_t& reflection_probe,
							   stock_world_renderer_prep_data_t&			  prep_data,
							   f32											  interpolation_alpha,
							   u8											  frame_index,
							   u32											  cluster_buffer_offset,
							   u32											  cluster_light_indices_buffer_offset,
							   u16*											  out_cull_view_indices);
		static void prep_probes(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, stock_world_renderer_prep_data_t& prep_data, const world_renderer_view_t& main_camera_view, f32 interpolation_alpha, u8 frame_index);
		static stock_world_renderer_reflection_allocation_t* prep_reflection_allocation(stock_world_renderer_context_t&					ctx,
																						const stock_world_renderer_snapshot_t&			snapshot,
																						stock_world_renderer_prep_data_t&				prep_data,
																						f32												interpolation_alpha,
																						u8												frame_index,
																						const stock_world_renderer_reflection_probe_t*& out_reflection_probe,
																						u16*											out_cull_view_indices);
		static void											 prep_shadow_buffer(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, u8 frame_index);
		static void											 prep_debug_buffer(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, u8 frame_index);
		static void											 prep_render_queues(stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, stock_world_renderer_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index);
		static u32											 prep_draw_packets(const stock_world_renderer_context_t&		   ctx,
																			   const stock_world_renderer_snapshot_t&		   snapshot,
																			   span_t<const stock_world_renderer_queue_item_t> draws,
																			   u32											   required_pass_mask,
																			   u32											   initial_variant_flags,
																			   u8											   frame_index,
																			   span_t<stock_world_renderer_draw_packet_t>	   packets,
																			   u32&											   draw_index);
		static void											 prep_render_pass_buffers(
			stock_world_renderer_context_t& ctx, const stock_world_renderer_snapshot_t& snapshot, const stock_world_renderer_prep_data_t& prep_data, const world_renderer_view_t& main_camera_view, const u32 (&light_counts)[4], u8 frame_index);
	};
}
