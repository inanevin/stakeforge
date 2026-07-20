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

#include "world_util.hpp"

#include <sfg/math/mat4x4.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	bool world_util_t::world_position_to_relative_position(const mat4x4_t& view_proj, const vec3f_t& world_position, vec2f_t& out_relative_position)
	{
		const vec4f_t clip = view_proj * vec4f_t(world_position.x, world_position.y, world_position.z, 1.0f);

		if (clip.w <= MATH_EPS)
			return false;

		const f32 inv_w		  = 1.0f / clip.w;
		out_relative_position = {
			clip.x * inv_w * 0.5f + 0.5f,
			0.5f - clip.y * inv_w * 0.5f,
		};
		return true;
	}

	bool world_util_t::relative_position_to_world_position(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, f32 ndc_depth, vec3f_t& out_world_position)
	{
		const f32 ndc_x = relative_position.x * 2.0f - 1.0f;
		const f32 ndc_y = 1.0f - relative_position.y * 2.0f;
		vec4f_t	  world = inv_view_proj * vec4f_t(ndc_x, ndc_y, ndc_depth, 1.0f);

		if (math::abs(world.w) <= MATH_EPS)
			return false;

		world /= world.w;
		out_world_position = {world.x, world.y, world.z};
		return true;
	}

	bool world_util_t::relative_position_to_world_ray(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, world_ray_t& out_ray)
	{
		vec3f_t far_point = vec3f_t::zero;

		if (!relative_position_to_world_position(inv_view_proj, relative_position, 1.0f, out_ray.origin) || !relative_position_to_world_position(inv_view_proj, relative_position, 0.0f, far_point))
			return false;

		out_ray.direction = (far_point - out_ray.origin).normalized();
		return !out_ray.direction.is_zero();
	}
}
