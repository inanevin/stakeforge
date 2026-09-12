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

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class world_t;
	struct vec2f_t;
	struct vec3f_t;

	u8 api_screen_world_to_screen(const world_t* world, const vec3f_t* world_position, vec2f_t* out_screen_position);
	u8 api_screen_screen_to_world(const world_t* world, const vec2f_t* screen_position, f32 ndc_depth, vec3f_t* out_world_position);

	struct script_api_screen_t
	{
		u32									  size			  = 0;
		u32									  version		  = 0;
		decltype(&api_screen_world_to_screen) world_to_screen = nullptr;
		decltype(&api_screen_screen_to_world) screen_to_world = nullptr;
	};

	const script_api_screen_t& get_script_api_screen();
}
