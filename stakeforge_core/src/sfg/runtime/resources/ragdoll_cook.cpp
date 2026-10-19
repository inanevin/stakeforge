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

#include "ragdoll_cook.hpp"

#include "ragdoll.hpp"
#include "ragdoll_def.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool ragdoll_cooker::cook_from_def(const ragdoll_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		if (!def.is_valid())
			return false;

		ostream_t def_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<ragdoll_def_t>::value, const_cast<ragdoll_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize ragdoll definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::ragdoll,
			.magic		 = ragdoll_loader_t::WIRE_MAGIC,
			.version	 = ragdoll_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		const resource_dependency_t skeleton_dependency{
			.handle = def.target_skeleton,
			.type	= resource_type_e::skeleton,
		};

		stream << skeleton_dependency;
		out_header.dependency_count = 1;

		if (def.physical_material != NULL_RESOURCE_HANDLE)
		{
			const resource_dependency_t material_dependency{
				.handle = def.physical_material,
				.type	= resource_type_e::physical_material,
			};

			stream << material_dependency;
			out_header.dependency_count++;
		}

		stream.write_raw(def_stream.get_raw(), def_stream.get_size());
		return true;
	}
}
