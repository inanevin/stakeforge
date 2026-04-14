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

#include "data/bitmask.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace sfg
{
	enum barrier_flags
	{
		baf_is_resource	 = 1 << 0,
		baf_is_texture	 = 1 << 1,
		baf_is_swapchain = 1 << 2,
		baf_is_uav		 = 1 << 3,
	};

	enum resource_state : u32
	{
		resource_state_common		   = 1 << 0,
		resource_state_vertex_cbv	   = 1 << 1,
		resource_state_index_buffer	   = 1 << 2,
		resource_state_render_target   = 1 << 3,
		resource_state_uav			   = 1 << 4,
		resource_state_depth_write	   = 1 << 5,
		resource_state_depth_read	   = 1 << 6,
		resource_state_non_ps_resource = 1 << 7,
		resource_state_ps_resource	   = 1 << 8,
		resource_state_indirect_arg	   = 1 << 9,
		resource_state_copy_dest	   = 1 << 10,
		resource_state_copy_source	   = 1 << 11,
		resource_state_resolve_dest	   = 1 << 12,
		resource_state_resolve_source  = 1 << 13,
		resource_state_generic_read	   = 1 << 14,
		resource_state_present		   = 1 << 15,
	};

	struct barrier_t
	{
		u32			  from_states = 0;
		u32			  to_states	  = 0;
		gfx_id_t		  resource_t  = 0;
		bitmask_t<u8> flags		  = 0;
	};

}
