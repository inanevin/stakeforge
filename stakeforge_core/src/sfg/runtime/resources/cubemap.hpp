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

#include "common_resources.hpp"
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	class cubemap_loader_t final
	{
	public:
		static inline constexpr u32 WIRE_MAGIC		 = make_resource_wire_magic('C', 'U', 'B', 'E');
		static inline constexpr u32 WIRE_VERSION	 = 1;
		static inline constexpr u8	FACE_COUNT		 = 6;
		static inline constexpr u8	MAX_MIPS		 = 16;
		static inline constexpr u8	MAX_SUBRESOURCES = FACE_COUNT * MAX_MIPS;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct cubemap_runtime_t
	{
		vec2u16_t size		= vec2u16_t::zero;
		format_e  format	= format_e::undefined;
		u8		  mip_count = 0;
	};

	struct cubemap_internals_t
	{
		render_resource_handle_t staging[cubemap_loader_t::FACE_COUNT] = {};
		render_resource_handle_t texture							   = {};
	};

	extern const resource_type_desc_t cubemap_resource_desc;
}
