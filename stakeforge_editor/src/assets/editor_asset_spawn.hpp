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

#include "world/editor_world_handle.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct editor_payload_t;

	struct editor_asset_spawn_desc_t
	{
		const editor_payload_t* payload	   = nullptr;
		vec2f_t					screen_pos = vec2f_t::zero;
		editor_world_handle_t	world	   = {};
		entity_id_t				parent	   = NULL_ENTITY_ID;
	};

	class editor_asset_spawn_t final
	{
	public:
		editor_asset_spawn_t()										 = delete;
		~editor_asset_spawn_t()										 = delete;
		editor_asset_spawn_t(const editor_asset_spawn_t&)			 = delete;
		editor_asset_spawn_t& operator=(const editor_asset_spawn_t&) = delete;

		static bool spawn_from_payload(const editor_asset_spawn_desc_t& desc);
	};
}
