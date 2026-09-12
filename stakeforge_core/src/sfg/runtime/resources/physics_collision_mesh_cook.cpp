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

#include "physics_collision_mesh_cook.hpp"

#include "physics_collision_mesh.hpp"
#include "physics_collision_mesh_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	bool physics_collision_mesh_cooker::cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		istream_t def_stream = serializer_t::load_from_file_compressed(full_path);
		if (def_stream.empty())
		{
			SFG_ERR("failed to read physics collision mesh definition file: {0}", full_path);
			return false;
		}

		physics_collision_mesh_def_t def = {};
		if (!reflection_registry_t::get().type_from_stream(type_id_t<physics_collision_mesh_def_t>::value, &def, nullptr, def_stream))
		{
			SFG_ERR("failed to deserialize physics collision mesh definition file: {0}", full_path);
			return false;
		}

		return cook_from_def(def, out_header, stream);
	}

	bool physics_collision_mesh_cooker::cook_from_def(const physics_collision_mesh_def_t& def, resource_header_t& out_header, ostream_t& stream, bool compress)
	{
		if (def.vertices.empty() || def.indices.empty() || def.indices.size() % 3 != 0)
		{
			SFG_ERR("physics collision mesh definition has invalid geometry");
			return false;
		}
		for (primitive_index index : def.indices)
		{
			if (index >= def.vertices.size())
			{
				SFG_ERR("physics collision mesh definition contains an invalid index");
				return false;
			}
		}

		ostream_t def_stream;
		if (!reflection_registry_t::get().type_to_stream(type_id_t<physics_collision_mesh_def_t>::value, const_cast<physics_collision_mesh_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize physics collision mesh definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::physics_collision_mesh,
			.magic		 = physics_collision_mesh_loader_t::WIRE_MAGIC,
			.version	 = physics_collision_mesh_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		if (!compress)
		{
			stream = std::move(def_stream);
			return true;
		}

		stream = compressor_t::compress(def_stream);
		if (stream.get_size() == 0)
		{
			SFG_ERR("failed to compress physics collision mesh payload");
			return false;
		}

		return true;
	}
}
