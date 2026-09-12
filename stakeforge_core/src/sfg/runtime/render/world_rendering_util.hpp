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

namespace sfg
{
	struct render_view_t;
	class world_render_context_t;
	class world_render_reflection_context_t;
	struct world_render_prep_data_t;
	struct world_render_reflection_allocation_t;
	struct world_render_reflection_probe_t;
	struct world_render_snapshot_t;

	class world_rendering_util_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		static void									 prep_entity_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index);
		static void									 prep_bone_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void									 prep_shadows(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha);
		static void									 prep_light_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, u32 (&light_counts)[4]);
		static void									 prep_probe(world_render_reflection_context_t&	   reflection_context,
																world_render_reflection_allocation_t&  allocation,
																const world_render_reflection_probe_t& reflection_probe,
																world_render_prep_data_t&			   prep_data,
																f32									   interpolation_alpha,
																u8									   frame_index,
																u32									   cluster_buffer_offset,
																u32									   cluster_light_indices_buffer_offset,
																u16*								   out_cull_view_indices);
		static void									 prep_probes(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha, u8 frame_index);
		static world_render_reflection_allocation_t* prep_reflection_allocation(
			world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, const world_render_reflection_probe_t*& out_reflection_probe, u16* out_cull_view_indices);
		static void prep_shadow_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index);
		static void prep_debug_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void prep_render_queues(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index);
		static void prep_render_pass_buffers(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, const u32 (&light_counts)[4], u8 frame_index);
	};
}
