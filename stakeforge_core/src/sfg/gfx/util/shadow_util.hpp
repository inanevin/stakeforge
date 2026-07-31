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

#include <sfg/data/inplace_vector.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct vec2u16_t;
	struct vec2f_t;
	class mat4x4_t;
	struct vec3f_t;

	namespace shadow_util_t
	{
		void get_world_space_ndc(const mat4x4_t& inv_view_proj, inplace_vector_t<vec4f_t, 8>& out_world_space, vec3f_t& out_center);
		void get_stable_directional_matrices(mat4x4_t&							 out_view,
											 mat4x4_t&							 out_proj,
											 const vec3f_t&						 light_forward,
											 const inplace_vector_t<vec4f_t, 8>& world_space_ndc,
											 const vec3f_t&						 receiver_center,
											 const vec2u16_t&					 resolution,
											 f32								 caster_extrusion,
											 vec2f_t&							 out_texel_size,
											 f32&								 out_near_plane,
											 f32&								 out_far_plane);
	}
}
