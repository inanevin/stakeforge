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
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	class world_logic_helper_t final
	{
	public:
		world_logic_helper_t()										 = default;
		~world_logic_helper_t()										 = default;
		world_logic_helper_t(const world_logic_helper_t&)			 = delete;
		world_logic_helper_t& operator=(const world_logic_helper_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void		sync_destroyers(f32 dt);
		void		sync_sprite_renderers();
		void		sync_reflection_probes(u64 tick_count);
		entity_id_t sync_main_camera();

	private:
		world_t* _world = nullptr;
	};
}
