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

#include "render_view.hpp"
#include "world_view.hpp"
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	void render_view_t::calculate(const world_view_t& world_view, const vec2u16_t& resolution, f32 interpolation_alpha)
	{
		const vec3f_t p	  = vec3f_t::lerp(world_view.prev_pos, world_view.pos, interpolation_alpha);
		const quat_t  rot = quat_t::slerp(world_view.prev_rot, world_view.rot, interpolation_alpha);

		view		  = mat4x4_t::view(rot, p);
		proj		  = mat4x4_t::perspective_reverse_z(world_view.fov_degrees, static_cast<f32>(resolution.x) / static_cast<f32>(resolution.y), world_view.near_plane, world_view.far_plane);
		view_proj	  = proj * view;
		frustum		  = frustum_t::extract(view_proj);
		inv_proj	  = proj.inverse();
		inv_view_proj = view_proj.inverse();
		pos			  = p;
		near_plane	  = world_view.near_plane;
		far_plane	  = world_view.far_plane;
		fov_rads	  = world_view.fov_degrees * DEG_2_RAD;
	}
}
