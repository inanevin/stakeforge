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
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	enum gpu_reflection_probe_flags_e : u32
	{
		gpu_reflection_probe_flag_global = 1 << 0,
	};

	struct gpu_reflection_probe_t
	{
		vec4f_t		position_blend_distance		  = vec4f_t::zero;
		vec4f_t		rotation					  = {0.0f, 0.0f, 0.0f, 1.0f};
		vec4f_t		extents_diffuse_intensity	  = vec4f_t::zero;
		f32			specular_intensity			  = 0.0f;
		u32			flags						  = 0;
		gpu_index_t radiance_texture_index		  = NULL_GPU_INDEX;
		gpu_index_t specular_texture_index		  = NULL_GPU_INDEX;
		gpu_index_t diffuse_sh_buffer_index		  = NULL_GPU_INDEX;
		u32			diffuse_sh_coefficient_offset = UINT32_MAX;
		u32			specular_mip_count			  = 0;
		u32			padding						  = 0;
	};

	static_assert(sizeof(gpu_reflection_probe_t) == 80);
}
