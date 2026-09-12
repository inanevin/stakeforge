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

#include "cubemap.hpp"
#include <sfg/gfx/common/texture_buffer.hpp>

namespace sfg
{
	struct cubemap_texture_block_t
	{
		texture_buffer_t buffers[cubemap_loader_t::MAX_SUBRESOURCES] = {};
		vec2u16_t		 size										 = vec2u16_t::zero;
		format_e		 format										 = format_e::undefined;
		u8				 mip_count									 = 0;
	};
}
