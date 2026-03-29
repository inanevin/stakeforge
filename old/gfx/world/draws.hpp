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

namespace SFG
{

	struct indexed_draw
	{
		u32		  entity_idx		 = 0;
		u32		  base_vertex		 = 0;
		u32		  index_count		 = 0;
		u32		  instance_count	 = 0;
		u32		  start_index		 = 0;
		u32		  start_instance	 = 0;
		gpu_index gpu_index_material = 0;
		gpu_index gpu_index_textures = 0;
		gfx_id	  pipeline			 = 0;
		gfx_id	  vertex_buffer		 = 0;
		gfx_id	  idx_buffer		 = 0;

		bool operator==(const indexed_draw& other)
		{
			return vertex_buffer == other.vertex_buffer && idx_buffer == other.idx_buffer && base_vertex == other.base_vertex && index_count == other.index_count && start_index == other.start_index && start_instance == other.start_instance &&
				   pipeline == other.pipeline && gpu_index_material == other.gpu_index_material && gpu_index_textures == other.gpu_index_textures;
		}
	};

}
