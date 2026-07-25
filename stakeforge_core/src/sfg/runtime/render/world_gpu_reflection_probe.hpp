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

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	enum gpu_reflection_probe_flags_e : u32
	{
		gpu_reflection_probe_flag_global = 1 << 0,
	};

	struct gpu_reflection_probe_t
	{
		vec4f_t		position_blend_distance		  = vec4f_t::zero;
		vec4f_t		rotation					  = {0.0f, 0.0f, 0.0f, 1.0f};
		vec4f_t		extents_diffuse_intensity	  = vec4f_t::zero;
		f32			specular_intensity			  = 0.0f;
		u32			flags						  = 0;
		gpu_index_t radiance_texture_index		  = NULL_GPU_INDEX;
		gpu_index_t specular_texture_index		  = NULL_GPU_INDEX;
		gpu_index_t diffuse_sh_buffer_index		  = NULL_GPU_INDEX;
		u32			diffuse_sh_coefficient_offset = UINT32_MAX;
		u32			specular_mip_count			  = 0;
		u32			padding						  = 0;
	};

	static_assert(sizeof(gpu_reflection_probe_t) == 80);
}
