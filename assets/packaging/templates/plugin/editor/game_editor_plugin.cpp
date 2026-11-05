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
#include "game_editor_plugin_type_registry.hpp"

#include <sfg/editor/plugin/game_editor_plugin_api.hpp>

namespace game::editor
{
	void load()
	{
	}

	void tick(sfg::world_t& world, f32 dt)
	{
	}

	void unload()
	{
	}
}

SFG_GAME_EDITOR_PLUGIN_ENTRY bool sfg_get_game_editor_plugin_callbacks(u32 api_version, sfg::game_editor_plugin_callbacks_t& callbacks)
{
	if (api_version != sfg::GAME_EDITOR_PLUGIN_API_VERSION)
		return false;

	callbacks = {
		.register_types = game::game_editor_plugin_type_registry_t::register_types,
		.load			= game::editor::load,
		.unload			= game::editor::unload,
		.tick_logic		= game::editor::tick,
	};

	return true;
}
