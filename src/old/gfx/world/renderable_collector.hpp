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

#include "gfx/world/renderable.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "data/vector.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view;
	class shader_direct;
	class draw_stream;
	class draw_stream_distance;
	class draw_stream_particle;
	class proxy_manager;

	class renderable_collector
	{
	public:
		renderable_collector() = delete;

		static void collect_mesh_instances(proxy_manager& pm, const view& view, vector<renderable_object>& out_objects);
		static void populate_draw_stream(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream& stream, u32 required_material_flags, u32 base_variant_flags, u8 frame_index, gfx_id override_shader = NULL_GFX_ID);
		static void populate_draw_stream(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream_distance& stream, u32 required_material_flags, u32 base_variant_flags, u8 frame_index, gfx_id override_shader = NULL_GFX_ID);
		static void populate_draw_stream_outline_filtered(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream& stream, u32 base_variant_flags, u8 frame_index, const shader_direct& direct, u32 target_world_id);
	};
}
