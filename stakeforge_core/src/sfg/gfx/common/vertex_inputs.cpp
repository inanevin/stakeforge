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
	void vertex_inputs_t::get_pos_normal_tangent_uv(vector_t<vertex_input_t>& out_inputs)
	{
		out_inputs.reserve(out_inputs.size() + 4);
		out_inputs.push_back({
			.name	= "POSITION",
			.offset = 0,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "NORMAL",
			.offset = sizeof(f32) * 3,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "TANGENT",
			.offset = sizeof(f32) * 6,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
		out_inputs.push_back({
			.name	= "TEXCOORD",
			.offset = sizeof(f32) * 10,
			.size	= sizeof(f32) * 2,
			.format = format_e::r32g32_sfloat,
		});
	}

	void vertex_inputs_t::get_pos_normal_tangent_uv_skinned(vector_t<vertex_input_t>& out_inputs)
	{
		out_inputs.reserve(out_inputs.size() + 6);
		out_inputs.push_back({
			.name	= "POSITION",
			.offset = 0,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "NORMAL",
			.offset = sizeof(f32) * 3,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "TANGENT",
			.offset = sizeof(f32) * 6,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
		out_inputs.push_back({
			.name	= "TEXCOORD",
			.offset = sizeof(f32) * 10,
			.size	= sizeof(f32) * 2,
			.format = format_e::r32g32_sfloat,
		});
		out_inputs.push_back({
			.name	= "BLENDWEIGHT",
			.offset = sizeof(f32) * 12,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
		out_inputs.push_back({
			.name	= "BLENDINDICES",
			.offset = sizeof(f32) * 16,
			.size	= sizeof(u32) * 4,
			.format = format_e::r32g32b32a32_uint,
		});
	}

	void vertex_inputs_t::get_line_3d(vector_t<vertex_input_t>& out_inputs)
	{
		out_inputs.reserve(out_inputs.size() + 4);
		out_inputs.push_back({
			.name	= "POSITION",
			.offset = 0,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "POSITION",
			.index	= 1,
			.offset = sizeof(f32) * 3,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "COLOR",
			.offset = sizeof(f32) * 6,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
		out_inputs.push_back({
			.name	= "POSITION",
			.index	= 2,
			.offset = sizeof(f32) * 10,
			.size	= sizeof(f32),
			.format = format_e::r32_sfloat,
		});
	}

	void vertex_inputs_t::get_pos_color(vector_t<vertex_input_t>& out_inputs)
	{
		out_inputs.reserve(out_inputs.size() + 2);
		out_inputs.push_back({
			.name	= "POSITION",
			.offset = 0,
			.size	= sizeof(f32) * 3,
			.format = format_e::r32g32b32_sfloat,
		});
		out_inputs.push_back({
			.name	= "COLOR",
			.offset = sizeof(f32) * 3,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
	}

	void vertex_inputs_t::get_editor_ui(vector_t<vertex_input_t>& out_inputs)
	{
		out_inputs.reserve(out_inputs.size() + 3);
		out_inputs.push_back({
			.name	= "POSITION",
			.offset = 0,
			.size	= sizeof(f32) * 2,
			.format = format_e::r32g32_sfloat,
		});
		out_inputs.push_back({
			.name	= "TEXCOORD",
			.offset = sizeof(f32) * 2,
			.size	= sizeof(f32) * 2,
			.format = format_e::r32g32_sfloat,
		});
		out_inputs.push_back({
			.name	= "COLOR",
			.offset = sizeof(f32) * 4,
			.size	= sizeof(f32) * 4,
			.format = format_e::r32g32b32a32_sfloat,
		});
	}
}
