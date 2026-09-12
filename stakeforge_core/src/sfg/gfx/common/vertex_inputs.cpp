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

#include "vertex_inputs.hpp"
#include <sfg/gfx/common/shader_description.hpp>

namespace sfg
{
	namespace
	{
		vertex_input_t make_input(const char* name, size_t offset, size_t size, format_e format, u8 index = 0, u8 location = 0)
		{
			vertex_input_t input = {};
			input.set_name(name);
			input.offset   = offset;
			input.size	   = size;
			input.format   = format;
			input.index	   = index;
			input.location = location;
			return input;
		}
	}

	void vertex_inputs_t::get_pos_normal_tangent_uv(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", 0, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("NORMAL", sizeof(f32) * 3, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("TANGENT", sizeof(f32) * 6, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 10, sizeof(f32) * 2, format_e::r32g32_sfloat));
	}

	void vertex_inputs_t::get_pos_normal_tangent_uv_skinned(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", 0, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("NORMAL", sizeof(f32) * 3, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("TANGENT", sizeof(f32) * 6, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 10, sizeof(f32) * 2, format_e::r32g32_sfloat));
		out_desc.add_input(make_input("BLENDWEIGHT", sizeof(f32) * 12, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
		out_desc.add_input(make_input("BLENDINDICES", sizeof(f32) * 16, sizeof(u32) * 4, format_e::r32g32b32a32_uint));
	}

	void vertex_inputs_t::get_line_3d(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", sizeof(f32) * 4, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("POSITION", sizeof(f32) * 7, sizeof(f32) * 3, format_e::r32g32b32_sfloat, 1));
		out_desc.add_input(make_input("COLOR", 0, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 10, sizeof(f32), format_e::r32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 11, sizeof(f32), format_e::r32_sfloat, 1));
	}

	void vertex_inputs_t::get_debug_text(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", sizeof(f32) * 4, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("POSITION", sizeof(f32) * 7, sizeof(f32) * 2, format_e::r32g32_sfloat, 1));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 9, sizeof(f32) * 2, format_e::r32g32_sfloat));
		out_desc.add_input(make_input("COLOR", 0, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 11, sizeof(f32), format_e::r32_sfloat, 1));
	}

	void vertex_inputs_t::get_pos_color(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", 0, sizeof(f32) * 3, format_e::r32g32b32_sfloat));
		out_desc.add_input(make_input("COLOR", sizeof(f32) * 3, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
	}

	void vertex_inputs_t::get_editor_ui(shader_desc_t& out_desc)
	{
		out_desc.add_input(make_input("POSITION", 0, sizeof(f32) * 2, format_e::r32g32_sfloat));
		out_desc.add_input(make_input("TEXCOORD", sizeof(f32) * 2, sizeof(f32) * 2, format_e::r32g32_sfloat));
		out_desc.add_input(make_input("COLOR", sizeof(f32) * 4, sizeof(f32) * 4, format_e::r32g32b32a32_sfloat));
	}
}
