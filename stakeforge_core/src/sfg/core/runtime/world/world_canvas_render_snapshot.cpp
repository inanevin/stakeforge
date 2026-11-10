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

#include "world_canvas_render_snapshot.hpp"
#include "core/runtime/world_renderer/world_render_config.hpp"

namespace sfg
{
	void world_canvas_draw_snapshot_t::init(const world_render_context_config_t& config)
	{
		const u32 vertex_count = config.canvas_vertex_max_bytes / sizeof(ui::vg_vertex_t);
		const u32 index_count  = config.canvas_index_max_bytes / sizeof(ui::vg_index_t);

		batches.init(vertex_count);
		parameters.init(vertex_count);
		vertices.init(vertex_count);
		indices.init(index_count);
		canvas_data.init(config.entity_max);
		owners.init(config.entity_max);
	}

	void world_canvas_draw_snapshot_t::uninit()
	{
		batches.uninit();
		parameters.uninit();
		vertices.uninit();
		indices.uninit();
		canvas_data.uninit();
		owners.uninit();
	}

	void world_canvas_render_snapshot_t::init(const world_render_context_config_t& config)
	{
		before_post_process.init(config);
		after_post_process.init(config);
		canvas_max = config.entity_max;
	}

	void world_canvas_render_snapshot_t::uninit()
	{
		before_post_process.uninit();
		after_post_process.uninit();
		canvas_max = 0;
	}
}
