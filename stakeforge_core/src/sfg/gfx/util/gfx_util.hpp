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
#include <sfg/gfx/common/descriptions.hpp>

namespace sfg
{
	enum gpu_constant_e : u8
	{
		constant_global0 = 0,
		constant_global1,
		constant_global2,
		constant_global3,
		constant_rp0,
		constant_rp1,
		constant_rp2,
		constant_rp3,
		constant_rp4,
		constant_rp5,
		constant_rp6,
		constant_rp7,
		constant_rp8,
		constant_rp9,
		constant_rp10,
		constant_rp11,
		constant_rp12,
		constant_rp13,
		constant_rp14,
		constant_rp15,
		constant_mat0,
		constant_mat1,
		constant_mat2,
		constant_mat3,
		constant_mat4,
		constant_mat5,
		constant_mat6,
		constant_mat7,
		constant_mat8,
		constant_mat9,
		constant_mat10,
		constant_mat11,
		constant_mat12,
		constant_mat13,
		constant_mat14,
		constant_mat15,
		constant_mat16,
		constant_obj0,
		constant_obj1,
		constant_obj2,
		constant_obj3,
		constant_obj4,
		constant_obj5,
		constant_obj6,
		constant_obj7,
		constant_obj8,
		constant_obj9,
		constant_obj10,
		constant_obj11,
		constant_obj12,
		constant_obj13,
		constant_max,
	};

	class gfx_util_t
	{
	public:
		static gfx_handle_t	  create_bind_layout_global(bool is_compute);
		static sampler_desc_t get_sampler_desc_anisotropic();
		static sampler_desc_t get_sampler_desc_anisotropic_repeat();
		static sampler_desc_t get_sampler_desc_linear();
		static sampler_desc_t get_sampler_desc_linear_repeat();
		static sampler_desc_t get_sampler_desc_nearest();
		static sampler_desc_t get_sampler_desc_nearest_repeat();
		static sampler_desc_t get_sampler_desc_shadow_2d();
		static sampler_desc_t get_sampler_desc_shadow_cube();
	};
}
