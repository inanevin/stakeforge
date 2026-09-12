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

namespace sfg
{
#define WORLD_RENDER_CLUSTER_TILE_SIZE		   32
#define WORLD_RENDER_CLUSTER_DEPTH_SLICE_COUNT 16
#define WORLD_RENDER_CLUSTER_LIGHT_CAPACITY	   32

	struct gpu_light_cluster_t
	{
		u32 light_offset = 0;
		u32 light_count	 = 0;
		u32 overflow	 = 0;
		u32 pad			 = 0;
	};

	static_assert(sizeof(gpu_light_cluster_t) == 16);
}
