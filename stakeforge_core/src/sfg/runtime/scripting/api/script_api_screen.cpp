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

#include "script_api_screen.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	u8 api_screen_world_to_screen(const world_t* world, const vec3f_t* world_position, vec2f_t* out_screen_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(world_position != nullptr);
		SFG_ASSERT(out_screen_position != nullptr);

		return world->get_screen().world_to_screen(*world_position, *out_screen_position) ? 1 : 0;
	}

	u8 api_screen_screen_to_world(const world_t* world, const vec2f_t* screen_position, f32 ndc_depth, vec3f_t* out_world_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(screen_position != nullptr);
		SFG_ASSERT(out_world_position != nullptr);

		return world->get_screen().screen_to_world(*screen_position, ndc_depth, *out_world_position) ? 1 : 0;
	}

	const script_api_screen_t& get_script_api_screen()
	{
		static const script_api_screen_t api{
			.size			 = static_cast<u32>(sizeof(script_api_screen_t)),
			.version		 = 1,
			.world_to_screen = api_screen_world_to_screen,
			.screen_to_world = api_screen_screen_to_world,
		};

		return api;
	}
}
