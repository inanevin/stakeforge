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

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	enum world_pass_flags_e
	{
		world_pass_flags_gbuffer	 = 1 << 0,
		world_pass_flags_forward	 = 1 << 1,
		world_pass_flags_depth		 = 1 << 2,
		world_pass_flags_shadow		 = 1 << 3,
		world_pass_flags_id			 = 1 << 4,
		world_pass_flags_reflections = 1 << 5,
	};

	enum world_draw_flags
	{
		wdf_none = 0,
	};

	SFG_DEFINE_TYPE_ID(world_pass_flags_e);

	struct world_pass_flags_reflection_t
	{
		world_pass_flags_reflection_t();
	};
	inline world_pass_flags_reflection_t g_reflect_world_pass_flags;
}
