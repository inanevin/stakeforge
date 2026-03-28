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
#include "common/size_definitions.hpp"
#include "math/matrix4x4.hpp"
#include "math/frustum.hpp"
#include "data/static_vector.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	struct view
	{
		frustum									view_frustum		 = {};
		matrix4x4								view_matrix			 = matrix4x4::identity;
		matrix4x4								proj_matrix			 = matrix4x4::identity;
		matrix4x4								inv_proj_matrix		 = matrix4x4::identity;
		matrix4x4								view_proj_matrix	 = matrix4x4::identity;
		matrix4x4								inv_view_proj_matrix = matrix4x4::identity;
		static_vector<f32, MAX_SHADOW_CASCADES> cascades;
		vector3									position			= vector3::zero;
		f32										near_plane			= 0.0f;
		f32										far_plane			= 0.0f;
		f32										fov_degrees			= 0.0f;
		u32										cascsades_gpu_index = 0;
	};
}
