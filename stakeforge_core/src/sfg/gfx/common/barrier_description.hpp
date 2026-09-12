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

#include "gfx_constants.hpp"
#include <sfg/data/bitmask.hpp>

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
		u32			  from_states	 = 0;
		u32			  to_states		 = 0;
		gfx_handle_t  resource_t	 = {};
		gfx_handle_t  texture_t		 = {};
		gfx_handle_t  swapchain_t	 = {};
		bitmask_t<u8> flags			 = 0;
		u8			  base_mip_level = 0;
		u8			  mip_count		 = 0;
	};

}
