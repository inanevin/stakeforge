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
#include "game_plugin_type_registry.hpp"
#include "game_world.hpp"

#include <sfg/core/runtime/plugin/game_plugin_api.hpp>

namespace game
{
	void load()
	{
	}

	void unload()
	{
	}
}

SFG_GAME_PLUGIN_ENTRY bool sfg_get_game_plugin_callbacks(u32 api_version, sfg::game_plugin_callbacks_t& callbacks)
{
	if (api_version != sfg::GAME_PLUGIN_API_VERSION)
		return false;

	callbacks = {
		.register_types		= game::game_plugin_type_registry_t::register_types,
		.load				= game::load,
		.unload				= game::unload,
		.begin_play			= game::game_world_t::begin_play,
		.end_play			= game::game_world_t::end_play,
		.tick_logic			= game::game_world_t::tick_logic,
		.tick_physics		= game::game_world_t::tick_physics,
		.tick_animation		= game::game_world_t::tick_animation,
		.key_event			= game::game_world_t::key_event,
		.mouse_button_event = game::game_world_t::mouse_button_event,
		.mouse_move_event	= game::game_world_t::mouse_move_event,
		.mouse_wheel_event	= game::game_world_t::mouse_wheel_event,
		.focus_event		= game::game_world_t::focus_event,
		.physics_contact	= game::game_world_t::physics_contact,
		.animation_event	= game::game_world_t::animation_event,
		.canvas_event		= game::game_world_t::canvas_event,
	};

	return true;
}
