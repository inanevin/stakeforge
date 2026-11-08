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

#include "core/common/sfg_exports.hpp"

#include "core/common/size_definitions.hpp"
#include "core/runtime/world/ecs_defs.hpp"

namespace sfg
{
	class world_t;

	class SFG_API world_util_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		world_util_t() = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static void		   sync_destroyers(world_t& world, f32 dt);
		static void		   sync_sprite_renderers(world_t& world);
		static void		   sync_reflection_probes(world_t& world, u64 tick_count);
		static entity_id_t get_active_camera(const world_t& world);
		static void		   sync_aabbs(world_t& world);
	};
}
