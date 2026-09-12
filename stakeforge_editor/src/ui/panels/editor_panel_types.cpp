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
#include "ui/panels/editor_panel_types.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	const char* editor_panel_type_to_string(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return "Entities";
		case editor_panel_type_e::assets:
			return "Assets";
		case editor_panel_type_e::log:
			return "Log";
		case editor_panel_type_e::world:
			return "World";
		case editor_panel_type_e::inspector:
			return "Inspector";
		case editor_panel_type_e::animation:
			return "Animation";
		case editor_panel_type_e::resources:
			return "Resources";
		case editor_panel_type_e::project_settings:
			return "Project Settings";
		case editor_panel_type_e::mesh_viewer:
			return "Mesh Viewer";
		case editor_panel_type_e::skeleton_viewer:
			return "Skeleton Viewer";
		case editor_panel_type_e::ragdoll_viewer:
			return "Ragdoll Viewer";
		case editor_panel_type_e::animation_library:
			return "Animation Library";
		case editor_panel_type_e::animation_graph:
			return "Animation Graph";
		default:
			return "";
		}
	}

	editor_panel_type_e editor_panel_type_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
		{
			const editor_panel_type_e type = static_cast<editor_panel_type_e>(i);
			if (TO_SID(editor_panel_type_to_string(type)) == id)
				return type;
		}
		return editor_panel_type_e::max;
	}
}
