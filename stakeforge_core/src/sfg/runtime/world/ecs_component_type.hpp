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
#include <sfg/data/bitmask.hpp>

namespace sfg
{
	enum ecs_component_type_flags_e : u32
	{
		ecs_component_type_flags_none = 0,
		ecs_component_type_flags_tag  = 1 << 0,
	};

	struct ecs_component_type_desc_t
	{
		sid_t		   type_id		  = 0;
		u32			   size			  = 0;
		u32			   alignment	  = 1;
		bitmask_t<u32> flags		  = ecs_component_type_flags_none;
		char		   debug_name[64] = {};
	};
}
