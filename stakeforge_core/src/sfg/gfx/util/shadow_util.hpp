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

#include <sfg/data/inplace_vector.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct vec2u16_t;
	struct vec2f_t;
	class mat4x4_t;
	struct vec3f_t;

	namespace shadow_util_t
	{
		void get_world_space_ndc(const mat4x4_t& inv_view_proj, inplace_vector_t<vec4f_t, 8>& out_world_space, vec3f_t& out_center);
		void get_stable_directional_matrices(mat4x4_t&							 out_view,
											 mat4x4_t&							 out_proj,
											 const vec3f_t&						 light_forward,
											 const inplace_vector_t<vec4f_t, 8>& world_space_ndc,
											 const vec3f_t&						 receiver_center,
											 const vec2u16_t&					 resolution,
											 f32								 caster_extrusion,
											 vec2f_t&							 out_texel_size,
											 f32&								 out_near_plane,
											 f32&								 out_far_plane);
	}
}
