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
