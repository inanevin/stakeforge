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
#include "curve_def.hpp"
#include <sfg/memory/chunk_handle.hpp>

namespace sfg
{
	struct curve_runtime_t
	{
		const vec4f_t*		  samples		= nullptr;
		chunk_handle32_t	  samples_chunk = {};
		u32					  sample_count	= 0;
		curve_type_e		  type			= curve_type_e::x;
		curve_interpolation_e interpolation = curve_interpolation_e::linear;

		vec4f_t sample(f32 time) const;
	};

	struct curve_internals_t
	{
		u32 reserved = 0;
	};

	class curve_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('C', 'U', 'R', 'V');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t curve_resource_desc;
}
