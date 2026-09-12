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
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	class world_t;

	struct editor_world_handle_tag_t
	{
	};

	using editor_world_handle_t		   = pool_handle_t<u32, editor_world_handle_tag_t>;
	using editor_world_tick_callback_t = void (*)(world_t& world, f32 delta_time, void* user_data);

	enum class editor_world_edit_type_e : u8
	{
		view_only,
		view_with_debug,
		full_control,
	};
}
