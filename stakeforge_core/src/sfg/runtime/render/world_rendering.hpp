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
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	class world_render_context_t;
	struct world_render_snapshot_t;

	class world_rendering_t final
	{
	public:
		static void render_world(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_bind_layout_handle global_layout);

	private:
		static void render_depth_prepass(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void render_gbuffer(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void render_lighting(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void render_forward(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
		static void render_post_process(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, u8 frame_index);
	};
}
