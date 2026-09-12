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

#include "vec3f.hpp"

namespace sfg
{
	struct plane_t
	{
		plane_t()  = default;
		~plane_t() = default;
		plane_t(f32 x, f32 y, f32 z, f32 dist) : normal(vec3f_t(x, y, z)), distance(dist) {};
		plane_t(const vec3f_t& n, f32 dist) : normal(n), distance(dist) {};

		void normalize();
		f32	 get_signed_distance(const vec3f_t& point) const;

		vec3f_t normal	 = vec3f_t::zero;
		f32		distance = 0.0f;
	};

}
