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
#include "ui/editor_payload_type.hpp"

namespace sfg
{
	const char* get_editor_payload_type_name(editor_payload_type_e type)
	{
		switch (type)
		{
		case editor_payload_type_e::panel:
			return "panel";
		case editor_payload_type_e::asset:
			return "asset";
		case editor_payload_type_e::asset_multi:
			return "asset_multi";
		case editor_payload_type_e::folder:
			return "folder";
		case editor_payload_type_e::folder_multi:
			return "folder_multi";
		case editor_payload_type_e::entity:
			return "entity";
		case editor_payload_type_e::entity_multi:
			return "entity_multi";
		default:
			return "";
		}
	}
}
