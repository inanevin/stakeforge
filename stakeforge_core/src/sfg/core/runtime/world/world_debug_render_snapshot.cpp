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

#include "world_debug_render_snapshot.hpp"
#include "world_debug_draw_config.hpp"

namespace sfg
{
	void world_debug_render_snapshot_t::init(const world_debug_draw_config_t& config)
	{
		line_vertices.init(config.line_vertex_max_count);
		line_indices.init(config.line_index_max_count);
		triangle_vertices.init(config.triangle_vertex_max_count);
		triangle_indices.init(config.triangle_index_max_count);
		text_vertices.init(config.text_vertex_max_count);
		text_indices.init(config.text_index_max_count);
		textures.init(config.texture_max_count);
	}

	void world_debug_render_snapshot_t::uninit()
	{
		line_vertices.uninit();
		line_indices.uninit();
		triangle_vertices.uninit();
		triangle_indices.uninit();
		text_vertices.uninit();
		text_indices.uninit();
		textures.uninit();
	}
}
