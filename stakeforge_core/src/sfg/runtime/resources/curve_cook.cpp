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

#include "curve_cook.hpp"
#include "curve.hpp"
#include "curve_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool curve_cooker::cook_from_def(const curve_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t def_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<curve_def_t>::value, const_cast<curve_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize curve definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::curve,
			.magic		 = curve_loader_t::WIRE_MAGIC,
			.version	 = curve_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		stream.write_raw(def_stream.get_raw(), def_stream.get_size());
		return true;
	}
}
