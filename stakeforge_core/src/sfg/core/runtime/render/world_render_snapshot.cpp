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

#include "core/runtime/render/world_render_snapshot.hpp"

#include "core/io/assert.hpp"
#include "core/io/log.hpp"
#include "core/memory/memory.hpp"

namespace sfg
{
	void world_render_snapshot_t::allocate_plugin_data(span_t<world_render_snapshot_t> snapshots, u32 size, u32 alignment)
	{
		for (const world_render_snapshot_t& snapshot : snapshots)
			SFG_ASSERT(snapshot.plugin_data == nullptr);

		if (size == 0)
			return;

		SFG_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0);

		for (world_render_snapshot_t& snapshot : snapshots)
		{
			snapshot.plugin_data = SFG_ALIGNED_MALLOC(alignment, size);

			if (snapshot.plugin_data == nullptr)
			{
				free_plugin_data(snapshots);
				SFG_ERR("failed to allocate {0} bytes for plugin render snapshot data", size);
				throw std::bad_alloc{};
			}

			SFG_MEMSET(snapshot.plugin_data, 0, size);
		}
	}

	void world_render_snapshot_t::free_plugin_data(span_t<world_render_snapshot_t> snapshots)
	{
		for (world_render_snapshot_t& snapshot : snapshots)
		{
			SFG_ALIGNED_FREE(snapshot.plugin_data);
			snapshot.plugin_data		   = nullptr;
			snapshot.plugin_snapshot_ready = false;
		}
	}
}
