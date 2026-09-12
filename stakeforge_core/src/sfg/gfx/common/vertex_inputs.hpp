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

namespace sfg
{
	struct shader_desc_t;

	class vertex_inputs_t
	{
	public:
		static void get_pos_normal_tangent_uv(shader_desc_t& out_desc);
		static void get_pos_normal_tangent_uv_skinned(shader_desc_t& out_desc);
		static void get_line_3d(shader_desc_t& out_desc);
		static void get_debug_text(shader_desc_t& out_desc);
		static void get_pos_color(shader_desc_t& out_desc);
		static void get_editor_ui(shader_desc_t& out_desc);
	};
}
