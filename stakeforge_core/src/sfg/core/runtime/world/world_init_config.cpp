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

#include "world_init_config.hpp"

namespace sfg
{
	world_init_config_t world_init_config_t::make_main(vec2u16_t render_resolution)
	{
		return {
			.physics = world_physics_config_t::make_main(),
			.debug_draw =
				{
					.line_vertex_max_count	   = 8192 * 4,
					.line_index_max_count	   = 8192 * 6,
					.triangle_vertex_max_count = 164000,
					.triangle_index_max_count  = 164000,
					.text_command_max_count	   = 256,
					.text_budget_bytes		   = 32768,
					.text_vertex_max_count	   = 16384,
					.text_index_max_count	   = 24576,
					.texture_max_count		   = 8192,
				},
			.render_resolution = render_resolution,
		};
	}

	world_init_config_t world_init_config_t::make_preview(vec2u16_t render_resolution)
	{
		world_init_config_t config = make_main(render_resolution);

		config.physics						 = world_physics_config_t::make_preview();
		config.particle_simulation.page_size = 4 * 1024;

		return config;
	}
}
