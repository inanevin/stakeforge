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

#include "world/editor_world_edit_context.hpp"
#include <sfg/data/span.hpp>

namespace sfg
{
	struct editor_action_menu_row_desc_t;

	class editor_entity_create_menu_t final
	{
	public:
		editor_entity_create_menu_t() = delete;

		static span_t<const editor_action_menu_row_desc_t> get_rows(bool allow_folder = true);
		static bool										   execute(u16 command, editor_world_handle_t world, entity_id_t parent = NULL_ENTITY_ID, editor_world_folder_handle_t folder = {});
	};
}
