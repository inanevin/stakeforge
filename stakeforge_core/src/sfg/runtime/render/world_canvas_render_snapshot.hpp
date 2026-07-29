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

#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	struct world_canvas_draw_snapshot_t
	{
		vector_t<ui::vg_draw_buffer_final_t> draw_buffers = {};
		vector_t<ui::vg_vertex_t>			 vertices	  = {};
		vector_t<ui::vg_index_t>			 indices	  = {};

		inline void clear()
		{
			draw_buffers.resize(0);
			vertices.resize(0);
			indices.resize(0);
		}

		inline ui::vg_draw_snapshot_t get_snapshot() const
		{
			return {
				.draw_buffers	   = draw_buffers.data(),
				.vertices		   = vertices.data(),
				.indices		   = indices.data(),
				.draw_buffer_count = static_cast<u32>(draw_buffers.size()),
				.vertex_count	   = static_cast<u32>(vertices.size()),
				.index_count	   = static_cast<u32>(indices.size()),
			};
		}
	};

	struct world_canvas_render_snapshot_t
	{
		world_canvas_draw_snapshot_t before_post_process = {};
		world_canvas_draw_snapshot_t after_post_process	 = {};

		inline void clear()
		{
			before_post_process.clear();
			after_post_process.clear();
		}
	};
}
