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

namespace sfg
{
	struct physics_collision_mesh_runtime_t
	{
		chunk_handle32_t vertices	  = {};
		chunk_handle32_t indices	  = {};
		chunk_handle32_t mesh_shape	  = {};
		u32				 vertex_count = 0;
		u32				 index_count  = 0;
	};

	class physics_collision_mesh_loader_t final
	{
	public:
		physics_collision_mesh_loader_t() = delete;

		static inline constexpr u32 WIRE_MAGIC	 = make_resource_wire_magic('P', 'C', 'M', 'H');
		static inline constexpr u32 WIRE_VERSION = 2;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static bool runtime_load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t physics_collision_mesh_resource_desc;
}
