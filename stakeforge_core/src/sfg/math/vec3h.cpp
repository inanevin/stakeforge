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

#include "vec3h.hpp"

namespace sfg
{
	const vec3h_t vec3h_t::zero	   = {0.0f, 0.0f, 0.0f};
	const vec3h_t vec3h_t::one	   = {1.0f, 1.0f, 1.0f};
	const vec3h_t vec3h_t::up	   = {0.0f, 1.0f, 0.0f};
	const vec3h_t vec3h_t::forward = {0.0f, 0.0f, -1.0f};
	const vec3h_t vec3h_t::right   = {1.0f, 0.0f, 0.0f};

}
