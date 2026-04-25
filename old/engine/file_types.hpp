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

#pragma once

#include "common/size_definitions.hpp"
#include "data/hash_map.hpp"
#include "data/string.hpp"

namespace sfg
{
	enum class file_type_t : u8
	{
		sfg_shader,
		sfg_tex,
		sfg_mesh,
		sfg_anim,
		sfg_skel,
		sfg_mat,
		sfg_font,
		sfg_aud,
		sfg_data,
		sfg_sampler,
		sfg_particle,
	};

	inline const hash_map_t<string_t, file_type_t> g_file_types = {
		{"sfg_shader", file_type_t::sfg_shader},
		{"sfg_tex", file_type_t::sfg_tex},
		{"sfg_mesh", file_type_t::sfg_mesh},
		{"sfg_anim", file_type_t::sfg_anim},
		{"sfg_skel", file_type_t::sfg_skel},
		{"sfg_mat", file_type_t::sfg_mat},
		{"sfg_font", file_type_t::sfg_font},
		{"sfg_aud", file_type_t::sfg_aud},
		{"sfg_data", file_type_t::sfg_data},
		{"sfg_sampler", file_type_t::sfg_sampler},
		{"sfg_particle", file_type_t::sfg_particle},
	};
}
