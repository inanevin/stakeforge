/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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
