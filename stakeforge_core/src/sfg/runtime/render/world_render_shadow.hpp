/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct gpu_shadow_view_t
	{
		mat4x4_t	view_proj	  = mat4x4_t::identity;
		vec4f_t		params0		  = vec4f_t::zero;
		vec4f_t		params1		  = vec4f_t::zero;
		vec4f_t		params2		  = vec4f_t::zero;
		gpu_index_t texture_index = NULL_GPU_INDEX;
		u32			slice		  = 0;
		u32			type		  = 0;
		u32			pad			  = 0;
	};

	struct world_render_shadow_view_t
	{
		vec2u16_t	 resolution		 = vec2u16_t::zero;
		gfx_handle_t texture		 = {};
		gpu_index_t	 texture_index	 = NULL_GPU_INDEX;
		u32			 light_index	 = UINT32_MAX;
		f32			 split_near		 = 0.0f;
		f32			 split_far		 = 0.0f;
		f32			 near_plane		 = 0.0f;
		f32			 far_plane		 = 0.0f;
		f32			 texel_world	 = 0.0f;
		f32			 fade			 = 1.0f;
		u16			 cull_view_index = UINT16_MAX;
		u8			 view_index		 = 0;
		u8			 type			 = 0;
	};
}
