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
	struct vec2u16_t;

	typedef u8 (*script_api_game_get_render_resolution_fn)(vec2u16_t& out_resolution);
	typedef u8 (*script_api_game_set_render_resolution_fn)(const vec2u16_t& resolution);
	typedef u8 (*script_api_game_load_world_fn)(sid_t world_name_hash);
	typedef u8 (*script_api_game_restart_world_fn)();
	typedef void (*script_api_game_quit_fn)();

	void set_script_api_game_callbacks(
		script_api_game_get_render_resolution_fn get_resolution, script_api_game_set_render_resolution_fn set_resolution, script_api_game_load_world_fn load_world, script_api_game_restart_world_fn restart_world, script_api_game_quit_fn quit);
	u8	 api_game_get_render_resolution(vec2u16_t* out_resolution);
	u8	 api_game_set_render_resolution(u16 width, u16 height);
	u8	 api_game_load_world(sid_t world_name_hash);
	u8	 api_game_restart_world();
	void api_game_quit();

	struct script_api_game_t
	{
		u32										  size					= 0;
		u32										  version				= 0;
		decltype(&api_game_get_render_resolution) get_render_resolution = nullptr;
		decltype(&api_game_set_render_resolution) set_render_resolution = nullptr;
		decltype(&api_game_load_world)			  load_world			= nullptr;
		decltype(&api_game_restart_world)		  restart_world			= nullptr;
		decltype(&api_game_quit)				  quit					= nullptr;
	};

	const script_api_game_t& get_script_api_game();
}
