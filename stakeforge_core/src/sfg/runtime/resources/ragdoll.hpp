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
#include "ragdoll_def.hpp"

namespace sfg
{
	using ragdoll_part_runtime_t = ragdoll_part_def_t;

	struct ragdoll_runtime_t
	{
		chunk_handle32_t  parts				= {};
		resource_handle_t target_skeleton	= NULL_RESOURCE_HANDLE;
		resource_handle_t physical_material = NULL_RESOURCE_HANDLE;
		f32				  gravity_factor	= 1.0f;
		f32				  linear_damping	= 0.05f;
		f32				  angular_damping	= 0.05f;
		u32				  part_count		= 0;
		u8				  allow_sleep		= 1;
	};

	struct ragdoll_internals_t
	{
		u32 reserved = 0;
	};

	class ragdoll_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('R', 'A', 'G', 'D');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t ragdoll_resource_desc;
}
