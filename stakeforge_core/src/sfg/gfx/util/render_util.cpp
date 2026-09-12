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

#include "render_util.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
#define WORLD_RENDER_RESOLUTION_MIN 64
#define WORLD_RENDER_RESOLUTION_MAX 8192

	void render_util_t::ensure_world_resolution(vec2u16_t& resolution)
	{
		resolution.x = static_cast<u16>(math::clamp<u32>(resolution.x, WORLD_RENDER_RESOLUTION_MIN, WORLD_RENDER_RESOLUTION_MAX));
		resolution.y = static_cast<u16>(math::clamp<u32>(resolution.y, WORLD_RENDER_RESOLUTION_MIN, WORLD_RENDER_RESOLUTION_MAX));
	}
}
