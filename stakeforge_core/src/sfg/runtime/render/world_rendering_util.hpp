/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	struct render_view_t;
	class world_render_context_t;
	struct world_render_prep_data_t;
	struct world_render_snapshot_t;

	class world_rendering_util_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		static void prep_entity_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index);
		static void prep_bone_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void prep_shadows(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha);
		static void prep_light_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, u32 (&light_counts)[4]);
		static void prep_probes(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, const render_view_t& main_camera_view, f32 interpolation_alpha, u8 frame_index);
		static void prep_shadow_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index);
		static void prep_debug_buffer(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const render_view_t& main_camera_view, u8 frame_index);
		static void prep_culls(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, u8 frame_index);
		static void prep_render_pass_buffers(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const render_view_t& main_camera_view, const u32 (&light_counts)[4], f32 interpolation_alpha, u8 frame_index);
	};
}
