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
#include "common/size_definitions.hpp"
#include "math/mat4x4.hpp"
#include "math/vec3f.hpp"
#include "math/vec2u16.hpp"
#include "math/vec2f.hpp"
#include "math/math.hpp"

namespace SFG
{
	void shadow_util::get_world_space_ndc(const mat4x4& inv_view_proj, static_vector_t<vec4f, 8>& out_world_space, vec3f& out_center)
	{
		for (u8 x = 0; x < 2; x++)
		{
			for (u8 y = 0; y < 2; y++)
			{
				for (u8 z = 0; z < 2; z++)
				{
					const vec4f v  = inv_view_proj * vec4f(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
					const vec4f ws = v / v.w;
					out_world_space.push_back(ws);
				}
			}
		}

		out_center = vec3f::zero;
		for (const vec4f& v : out_world_space)
			out_center += vec3f(v.x, v.y, v.z);
		out_center /= static_cast<f32>(out_world_space.size());
	}

	void shadow_util::get_lightspace_projection(mat4x4& out_proj, const mat4x4& light_view, const static_vector_t<vec4f, 8>& world_space_ndc, const vec2u16& resolution, vec2f& out_texel_size)
	{
		{

			f32 min_x = std::numeric_limits<f32>::max();
			f32 max_x = std::numeric_limits<f32>::lowest();
			f32 min_y = std::numeric_limits<f32>::max();
			f32 max_y = std::numeric_limits<f32>::lowest();
			f32 min_z = std::numeric_limits<f32>::max();
			f32 max_z = std::numeric_limits<f32>::lowest();
			for (const auto& v : world_space_ndc)
			{
				const auto trf = light_view * v;
				min_x		   = math::min(min_x, trf.x);
				max_x		   = math::max(max_x, trf.x);
				min_y		   = math::min(min_y, trf.y);
				max_y		   = math::max(max_y, trf.y);
				min_z		   = math::min(min_z, trf.z);
				max_z		   = math::max(max_z, trf.z);
			}

			// f32 orthoWidth  = max_x - min_x;
			// f32 orthoHeight = max_y - min_y;
			//
			// // Texel size in light space:
			// f32 texelX = orthoWidth / static_cast<f32>(resolution.x);
			// f32 texelY = orthoHeight / static_cast<f32>(resolution.y);
			//
			// // Center before snapping
			// vec2f center = {0.5f * (min_x + max_x), 0.5f * (min_y + max_y)};
			//
			// // Snap center to texel grid
			// center.x = floor(center.x / texelX + 0.5f) * texelX;
			// center.y = floor(center.y / texelY + 0.5f) * texelY;
			//
			// // Rebuild min/max using snapped center (KEEP SIZE THE SAME)
			// min_x = center.x - 0.5f * orthoWidth;
			// max_x = center.x + 0.5f * orthoWidth;
			// min_y = center.y - 0.5f * orthoHeight;
			// max_y = center.y + 0.5f * orthoHeight;

			constexpr f32 zMult = 10.0f;

			if (min_z < 0)
			{
				min_z *= zMult;
			}
			else
			{
				min_z /= zMult;
			}
			if (max_z < 0)
			{
				max_z /= zMult;
			}
			else
			{
				max_z *= zMult;
			}

			f32 near_dist = -max_z;
			f32 far_dist  = -min_z;

			out_proj	   = mat4x4::ortho(min_x, max_x, max_y, min_y, near_dist, far_dist);
			out_texel_size = vec2f(max_x - min_x / static_cast<f32>(resolution.x), max_y - min_y / static_cast<f32>(resolution.y));
		}

		//
	}
}
