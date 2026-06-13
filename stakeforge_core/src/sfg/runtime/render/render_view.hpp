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

#include <sfg/math/frustum.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	struct vec2u16_t;
	struct world_view_t;

	struct render_view_t
	{
		frustum_t frustum		= {};
		mat4x4_t  view			= {};
		mat4x4_t  proj			= {};
		mat4x4_t  inv_proj		= {};
		mat4x4_t  view_proj		= {};
		mat4x4_t  inv_view_proj = {};
		vec3f_t	  pos			= vec3f_t::zero;
		f32		  near_plane	= 0.0f;
		f32		  far_plane		= 0.0f;
		f32		  fov_rads		= 0.0f;

		void calculate(const world_view_t& world_view, const vec2u16_t& resolution, f32 interpolation_alpha);
	};
}
