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
#include <sfg/data/bitmask.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/resources/material_limits.hpp>
#include <sfg/runtime/resources/shader_limits.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	struct alignas(64) world_render_material_t
	{
		render_resource_handle_t material_textures[SFG_MATERIAL_MAX_TEXTURES];
		render_resource_handle_t material_samplers[SFG_MATERIAL_MAX_TEXTURES];
		render_resource_handle_t material_buffers[BACK_BUFFER_COUNT]	= {};
		render_resource_handle_t psos[SFG_SHADER_MAX_PSO_VARIANTS]		= {};
		u32						 pso_flags[SFG_SHADER_MAX_PSO_VARIANTS] = {};
		u32						 pso_count								= 0;
		u32						 texture_count							= 0;
		u32						 pass_mask								= 0;
		u32						 particle_variant_flags					= 0;
		u8						 double_sided							= 0;
		u8						 use_alpha_cutoff						= 0;

		inline render_resource_handle_t find_pso(bitmask_t<u32> flags) const
		{
			const u32 want = flags.value();
			for (u8 i = 0; i < pso_count; ++i)
			{
				if (pso_flags[i] == want)
					return psos[i];
			}
			return {};
		}
	};
}
