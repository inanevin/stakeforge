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

#include "skeleton_cook.hpp"

#include "skeleton.hpp"
#include "skeleton_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool skeleton_cooker::cook_from_def(const skeleton_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		if (!def.is_evaluation_order_valid())
		{
			SFG_ERR("skeleton evaluation order is invalid");
			return false;
		}

		ostream_t skeleton_stream = {};
		if (!reflection_registry_t::get().type_to_stream(type_id_t<skeleton_def_t>::value, const_cast<skeleton_def_t*>(&def), nullptr, skeleton_stream))
		{
			SFG_ERR("failed to serialize skeleton definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::skeleton,
			.magic		 = skeleton_loader_t::WIRE_MAGIC,
			.version	 = skeleton_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(skeleton_stream.get_raw(), skeleton_stream.get_size()),
		};

		stream.write_raw(skeleton_stream.get_raw(), skeleton_stream.get_size());
		return true;
	}
}
