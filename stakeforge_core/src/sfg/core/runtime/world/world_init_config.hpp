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

#include "core/common/size_definitions.hpp"
#include "core/math/vec2u16.hpp"
#include "core/runtime/physics/physics_config.hpp"
#include "core/runtime/world/world_debug_draw_config.hpp"
#include "core/runtime/world/world_particle_simulation_config.hpp"

namespace sfg
{
	struct world_init_config_t
	{
		world_physics_config_t			   physics						 = {};
		world_debug_draw_config_t		   debug_draw					 = {};
		world_particle_simulation_config_t particle_simulation			 = {};
		vec2u16_t						   render_resolution			 = vec2u16_t(512, 512);
		u32								   animation_processor_page_size = 512 * 1024;
	};
}
