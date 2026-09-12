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
#define ENTITIES_INITIAL_ROW_CAPACITY 64
#define ENTITIES_INDENT_MULT		  2.0f

	enum entity_action_menu_command_e : u16
	{
		entity_action_menu_create_empty = 1,
		entity_action_menu_create_folder,
		entity_action_menu_create_cube,
		entity_action_menu_create_sphere,
		entity_action_menu_create_cylinder,
		entity_action_menu_create_capsule,
		entity_action_menu_create_plane,
		entity_action_menu_duplicate,
		entity_action_menu_hide,
		entity_action_menu_show_alone,
		entity_action_menu_delete,
		entity_action_menu_rename_folder,
		entity_action_menu_change_folder_color,
		entity_action_menu_delete_folder,
	};
}
