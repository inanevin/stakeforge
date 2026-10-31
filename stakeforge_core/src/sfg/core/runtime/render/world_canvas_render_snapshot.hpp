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

#include "core/data/vector.hpp"
#include "core/runtime/ui/vg/vg_canvas.hpp"

namespace sfg
{
	struct world_canvas_draw_snapshot_t
	{
		vector_t<ui::vg_draw_batch_t>	   batches	  = {};
		vector_t<ui::ui_draw_parameters_t> parameters = {};
		vector_t<ui::vg_vertex_t>		   vertices	  = {};
		vector_t<ui::vg_index_t>		   indices	  = {};

		inline void clear()
		{
			batches.resize(0);
			parameters.resize(0);
			vertices.resize(0);
			indices.resize(0);
		}

		inline ui::vg_draw_snapshot_t get_snapshot() const
		{
			return {
				.batches		 = batches.data(),
				.vertices		 = vertices.data(),
				.indices		 = indices.data(),
				.parameters		 = parameters.data(),
				.batch_count	 = static_cast<u32>(batches.size()),
				.vertex_count	 = static_cast<u32>(vertices.size()),
				.index_count	 = static_cast<u32>(indices.size()),
				.parameter_count = static_cast<u32>(parameters.size()),
			};
		}
	};

	struct world_canvas_render_snapshot_t
	{
		world_canvas_draw_snapshot_t before_post_process = {};
		world_canvas_draw_snapshot_t after_post_process	 = {};

		inline void clear()
		{
			before_post_process.clear();
			after_post_process.clear();
		}
	};
}
