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
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/physics/physics_config.hpp>
#include <sfg/runtime/world/world_debug_draw_config.hpp>

namespace sfg
{
	struct world_init_config_t
	{
		world_debug_draw_config_t debug_draw			  = {};
		physics_runtime_config_t  physics				  = {};
		vec2u16_t				  render_resolution		  = vec2u16_t(512, 512);
		u32						  render_entity_max		  = 256;
		u32						  render_bone_max		  = 256;
		u32						  render_bone_reserve	  = 256;
		u32						  component_table_reserve = 64;
		u32						  free_list_reserve		  = 1024;
		u32						  used_resource_reserve	  = 512;
		u32						  text_allocation_reserve = 1024;
		u32						  text_byte_reserve		  = 64 * 1024;
		bool					  physics_enabled		  = false;
	};
}
