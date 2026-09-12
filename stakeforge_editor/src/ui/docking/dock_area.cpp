/*
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
#include "ui/docking/dock_area.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	const char* dock_node_type_to_string(dock_node_type_e type)
	{
		switch (type)
		{
		case dock_node_type_e::leaf:
			return "leaf";
		case dock_node_type_e::split:
			return "split";
		}
		return "leaf";
	}

	dock_node_type_e dock_node_type_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);
		if (id == TO_SID("split"))
			return dock_node_type_e::split;
		return dock_node_type_e::leaf;
	}

	const char* dock_split_direction_to_string(dock_split_direction_e direction)
	{
		switch (direction)
		{
		case dock_split_direction_e::horizontal:
			return "horizontal";
		case dock_split_direction_e::vertical:
			return "vertical";
		}
		return "horizontal";
	}

	dock_split_direction_e dock_split_direction_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);
		if (id == TO_SID("vertical"))
			return dock_split_direction_e::vertical;
		return dock_split_direction_e::horizontal;
	}
}
