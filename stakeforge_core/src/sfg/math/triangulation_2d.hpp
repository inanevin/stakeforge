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

#include "vec2f.hpp"
#include "vec3f.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct triangle_indices_t
	{
		u32 indices[3] = {};
	};

	namespace math
	{
		bool	triangulate_2d(span_t<const vec2f_t> points, vector_t<triangle_indices_t>& out_triangles);
		bool	triangle_barycentric_2d(const vec2f_t& point, const vec2f_t& a, const vec2f_t& b, const vec2f_t& c, vec3f_t& out_weights);
		vec3f_t closest_triangle_barycentric_2d(const vec2f_t& point, const vec2f_t& a, const vec2f_t& b, const vec2f_t& c);
	}
}
