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

#include "script_api_game.hpp"

#include <sfg/gfx/util/render_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	namespace
	{
		script_api_game_get_render_resolution_fn g_get_render_resolution = nullptr;
		script_api_game_set_render_resolution_fn g_set_render_resolution = nullptr;
		script_api_game_load_world_fn			 g_load_world			 = nullptr;
		script_api_game_restart_world_fn		 g_restart_world		 = nullptr;
		script_api_game_quit_fn					 g_quit					 = nullptr;
	}

	void set_script_api_game_callbacks(
		script_api_game_get_render_resolution_fn get_resolution, script_api_game_set_render_resolution_fn set_resolution, script_api_game_load_world_fn load_world, script_api_game_restart_world_fn restart_world, script_api_game_quit_fn quit)
	{
		g_get_render_resolution = get_resolution;
		g_set_render_resolution = set_resolution;
		g_load_world			= load_world;
		g_restart_world			= restart_world;
		g_quit					= quit;
	}

	u8 api_game_get_render_resolution(vec2u16_t* out_resolution)
	{
		SFG_ASSERT(out_resolution != nullptr);

		*out_resolution = vec2u16_t::zero;

		if (g_get_render_resolution == nullptr)
			return 0;

		return g_get_render_resolution(*out_resolution);
	}

	u8 api_game_set_render_resolution(u16 width, u16 height)
	{
		if (g_set_render_resolution == nullptr)
			return 0;

		vec2u16_t resolution{width, height};
		render_util_t::ensure_world_resolution(resolution);
		return g_set_render_resolution(resolution);
	}

	u8 api_game_load_world(sid_t world_name_hash)
	{
		if (g_load_world == nullptr)
			return 0;

		return g_load_world(world_name_hash);
	}

	u8 api_game_restart_world()
	{
		if (g_restart_world == nullptr)
			return 0;

		return g_restart_world();
	}

	void api_game_quit()
	{
		if (g_quit == nullptr)
			return;

		g_quit();
	}

	const script_api_game_t& get_script_api_game()
	{
		static const script_api_game_t api{
			.size				   = static_cast<u32>(sizeof(script_api_game_t)),
			.version			   = 4,
			.get_render_resolution = api_game_get_render_resolution,
			.set_render_resolution = api_game_set_render_resolution,
			.load_world			   = api_game_load_world,
			.restart_world		   = api_game_restart_world,
			.quit				   = api_game_quit,
		};

		return api;
	}
}
