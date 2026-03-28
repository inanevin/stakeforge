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

namespace SFG
{
	class mat4x4;
	class mat3x3;
	class vec3f;

	enum class frustum_result
	{
		inside,
		outside,
		intersects,
	};

	struct aabb;
	struct plane;

	struct frustum
	{
		static frustum_result test(const frustum& fr, const aabb& local_box);
		static frustum_result test(const frustum& fr, const vec3f& position, f32 sphere_radius);
		static frustum_result test(const frustum& fr, const aabb& local_box, const mat3x3& linear_model, const vec3f& position);
		static frustum_result classify_obb_vs_plane(const plane& p, const vec3f& c_local, const vec3f& e_local, const mat3x3& linear_model, const vec3f& position);
		static frustum		  extract(const mat4x4& view_proj);

		plane left	 = {};
		plane right	 = {};
		plane bottom = {};
		plane top	 = {};
		plane near	 = {};
		plane far	 = {};
	};
}
