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

#include "vec2i16.hpp"
#include "math.hpp"

namespace sfg
{
	vec2i16_t vec2i16_t::zero = {0, 0};
	vec2i16_t vec2i16_t::one  = {1, 1};

	vec2i16_t vec2i16_t::clamp(const vec2i16_t& v, const vec2i16_t& min, const vec2i16_t& max)
	{
		return {math::clamp(v.x, min.x, max.x), math::clamp(v.y, min.y, max.y)};
	}

}
