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

#include <sfg/common/size_definitions.hpp>
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
		render_resource_handle_t material_buffer						= {};
		render_resource_handle_t psos[SFG_SHADER_MAX_PSO_VARIANTS]		= {};
		u32						 pso_flags[SFG_SHADER_MAX_PSO_VARIANTS] = {};
		u32						 pso_count								= 0;
		u32						 texture_count							= 0;
		u32						 pass_mask								= 0;

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
