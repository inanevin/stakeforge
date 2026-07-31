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
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct gpu_shadow_view_t
	{
		mat4x4_t	view_proj	  = mat4x4_t::identity;
		vec4f_t		params0		  = vec4f_t::zero;
		vec4f_t		params1		  = vec4f_t::zero;
		vec4f_t		params2		  = vec4f_t::zero;
		gpu_index_t texture_index = NULL_GPU_INDEX;
		u32			slice		  = 0;
		u32			type		  = 0;
		u32			pad			  = 0;
	};

	struct world_render_shadow_view_t
	{
		vec2u16_t	 resolution		 = vec2u16_t::zero;
		gfx_handle_t texture		 = {};
		gpu_index_t	 texture_index	 = NULL_GPU_INDEX;
		u32			 light_index	 = UINT32_MAX;
		f32			 split_near		 = 0.0f;
		f32			 split_far		 = 0.0f;
		f32			 near_plane		 = 0.0f;
		f32			 far_plane		 = 0.0f;
		f32			 texel_world		 = 0.0f;
		f32			 fade			 = 1.0f;
		u16			 cull_view_index = UINT16_MAX;
		u8			 view_index		 = 0;
		u8			 type			 = 0;
	};
}
