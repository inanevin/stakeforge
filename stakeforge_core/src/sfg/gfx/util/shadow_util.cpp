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

#include "shadow_util.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/math.hpp>

namespace sfg
{
	void shadow_util_t::get_world_space_ndc(const mat4x4_t& inv_view_proj, inplace_vector_t<vec4f_t, 8>& out_world_space, vec3f_t& out_center)
	{
		for (u8 x = 0; x < 2; x++)
		{
			for (u8 y = 0; y < 2; y++)
			{
				for (u8 z = 0; z < 2; z++)
				{
					const vec4f_t v	 = inv_view_proj * vec4f_t(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
					const vec4f_t ws = v / v.w;
					out_world_space.push_back(ws);
				}
			}
		}

		out_center = vec3f_t::zero;
		for (const vec4f_t& v : out_world_space)
			out_center += vec3f_t(v.x, v.y, v.z);
		out_center /= static_cast<f32>(out_world_space.size());
	}

	void shadow_util_t::get_stable_directional_matrices(mat4x4_t&							out_view,
														mat4x4_t&							out_proj,
														const vec3f_t&						light_forward,
														const inplace_vector_t<vec4f_t, 8>& world_space_ndc,
														const vec3f_t&						receiver_center,
														const vec2u16_t&					resolution,
														f32									caster_extrusion,
														vec2f_t&							out_texel_size,
														f32&								out_near_plane,
														f32&								out_far_plane)
	{
		constexpr u16 CASCADE_GUARD_TEXELS		  = 2;
		constexpr f32 CASCADE_RADIUS_QUANTIZATION = 16.0f;
		constexpr f32 CASCADE_DEPTH_GUARD		  = 1.0f;

		f32 receiver_radius = 0.0f;
		for (const vec4f_t& corner : world_space_ndc)
			receiver_radius = math::max(receiver_radius, vec3f_t::distance(receiver_center, {corner.x, corner.y, corner.z}));

		// A quantized bounding sphere makes the projection size independent of camera
		// rotation and suppresses tiny floating-point changes in the reconstructed corners.
		receiver_radius = math::ceil(receiver_radius * CASCADE_RADIUS_QUANTIZATION) / CASCADE_RADIUS_QUANTIZATION;

		const u16 usable_width	 = math::max<u16>(1, resolution.x > CASCADE_GUARD_TEXELS * 2 ? resolution.x - CASCADE_GUARD_TEXELS * 2 : 1);
		const u16 usable_height	 = math::max<u16>(1, resolution.y > CASCADE_GUARD_TEXELS * 2 ? resolution.y - CASCADE_GUARD_TEXELS * 2 : 1);
		const f32 guarded_radius = receiver_radius * math::max(static_cast<f32>(resolution.x) / usable_width, static_cast<f32>(resolution.y) / usable_height);

		out_texel_size = {guarded_radius * 2.0f / math::max<f32>(resolution.x, 1.0f), guarded_radius * 2.0f / math::max<f32>(resolution.y, 1.0f)};

		const vec3f_t forward = light_forward.normalized();
		const vec3f_t up	  = math::abs(vec3f_t::dot(forward, vec3f_t::up)) > 0.95f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t view_z  = -forward;
		const vec3f_t view_x  = vec3f_t::cross(up, view_z).normalized();
		const vec3f_t view_y  = vec3f_t::cross(view_z, view_x);

		// Snap absolute light-basis coordinates. Snapping a center after constructing a
		// center-relative view would always snap zero and would not stabilize the matrix.
		const f32	  center_x		   = vec3f_t::dot(receiver_center, view_x);
		const f32	  center_y		   = vec3f_t::dot(receiver_center, view_y);
		const f32	  snapped_center_x = math::floor(center_x / out_texel_size.x + 0.5f) * out_texel_size.x;
		const f32	  snapped_center_y = math::floor(center_y / out_texel_size.y + 0.5f) * out_texel_size.y;
		const vec3f_t snapped_center   = receiver_center + view_x * (snapped_center_x - center_x) + view_y * (snapped_center_y - center_y);

		caster_extrusion			= math::max(caster_extrusion, 0.0f);
		const f32	  view_distance = receiver_radius + caster_extrusion + CASCADE_DEPTH_GUARD;
		const vec3f_t view_pos		= snapped_center - forward * view_distance;

		out_near_plane = CASCADE_DEPTH_GUARD;
		out_far_plane  = CASCADE_DEPTH_GUARD * 2.0f + caster_extrusion + receiver_radius * 2.0f;
		out_view	   = mat4x4_t::look_at(view_pos, snapped_center, up);
		out_proj	   = mat4x4_t::ortho(-guarded_radius, guarded_radius, guarded_radius, -guarded_radius, out_near_plane, out_far_plane);
	}
}
