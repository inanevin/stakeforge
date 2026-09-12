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

#define NOMINMAX
#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
#define BACK_BUFFER_COUNT 3
#define FRAME_LATENCY	  2
	inline constexpr u8 TEXTURE_MAX_VIEWS = 16;

	// 0 discrete, 1 integratd
#define GPU_DEVICE 0

	typedef unsigned short gfx_id_t;
	typedef u32			   primitive_index;
	typedef unsigned int   gpu_index_t;

	struct gfx_handle_tag
	{
	};

	typedef pool_handle_t<gfx_id_t, gfx_handle_tag> gfx_handle_t;

#define NULL_GFX_ID	   (unsigned short)0xFFFF
#define NULL_GPU_INDEX (unsigned int)0xFFFFFFFF
}
