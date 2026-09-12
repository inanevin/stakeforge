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

#include "ui/panels/editor_panel_types.hpp"

namespace sfg
{
	class editor_panel_t;

	struct editor_panel_type_desc_t
	{
		editor_panel_type_e type					  = editor_panel_type_e::max;
		bool				allows_multiple_instances = false;
	};

	class editor_panel_factory_t final
	{
	public:
		static editor_panel_t*				   create_panel(editor_panel_type_e type);
		static void							   delete_panel(editor_panel_t* panel);
		static const editor_panel_type_desc_t& get_desc(editor_panel_type_e type);
	};
}
