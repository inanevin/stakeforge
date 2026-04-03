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

#include "plane.hpp"

#undef near
#undef far

namespace sfg
{
	class mat4x4_t;
	class mat3x3_t;
	class vec3f_t;

	enum class frustum_result
	{
		inside,
		outside,
		intersects,
	};

	struct aabb_t;
	struct plane_t;

	struct frustum_t
	{
		static frustum_result test(const frustum_t& fr, const aabb_t& local_box);
		static frustum_result test(const frustum_t& fr, const vec3f_t& position, f32 sphere_radius);
		static frustum_result test(const frustum_t& fr, const aabb_t& local_box, const mat3x3_t& linear_model, const vec3f_t& position);
		static frustum_result classify_obb_vs_plane(const plane_t& p, const vec3f_t& c_local, const vec3f_t& e_local, const mat3x3_t& linear_model, const vec3f_t& position);
		static frustum_t	  extract(const mat4x4_t& view_proj);

		plane_t left   = {};
		plane_t right  = {};
		plane_t bottom = {};
		plane_t top	   = {};
		plane_t near   = {};
		plane_t far	   = {};
	};
}
