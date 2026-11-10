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

#include "core/math/vec2u16.hpp"

namespace sfg
{
	struct SFG_API world_render_context_config_t
	{
		vec2u16_t size					  = vec2u16_t::zero;
		u32		  entity_max			  = 0;
		u32		  mesh_draw_max			  = 0;
		u32		  material_max			  = 0;
		u32		  sprite_max			  = 0;
		u32		  particle_max			  = 0;
		u32		  bone_max				  = 0;
		u32		  light_max				  = 0;
		u32		  reflection_probe_max	  = 0;
		u32		  canvas_vertex_max_bytes = 1u << 20;
		u32		  canvas_index_max_bytes  = 1u << 20;
		u16		  shadow_view_max		  = 0;
		u8		  enable_ssao			  = 1;
		u8		  enable_bloom			  = 1;

		static world_render_context_config_t make_main(vec2u16_t render_resolution);
		static world_render_context_config_t make_preview(vec2u16_t render_resolution);
	};

}
