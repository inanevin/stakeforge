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
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/resources/vertex.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct world_debug_draw_texture_t
	{
		vec4f_t					 color		   = vec4f_t::zero;
		vec3f_t					 position	   = vec3f_t::zero;
		render_resource_handle_t texture	   = {};
		vec2f_t					 size_px	   = vec2f_t::zero;
		vec2f_t					 screen_offset = vec2f_t::zero;
		entity_id_t				 entity_id	   = NULL_ENTITY_ID;
		u32						 flags		   = 0;
	};

	struct world_debug_draw_snapshot_t
	{
		vector_t<vertex_debug_line_t>		 line_vertices;
		vector_t<primitive_index>			 line_indices;
		vector_t<vertex_debug_triangle_t>	 triangle_vertices;
		vector_t<primitive_index>			 triangle_indices;
		vector_t<vertex_debug_text_t>		 text_vertices;
		vector_t<primitive_index>			 text_indices;
		vector_t<world_debug_draw_texture_t> textures;
	};
}
