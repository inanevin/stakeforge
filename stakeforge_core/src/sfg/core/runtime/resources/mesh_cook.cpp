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

#include "core/runtime/resources/mesh_cook.hpp"

#include "core/runtime/resources/mesh.hpp"
#include "core/runtime/resources/mesh_def.hpp"
#include "core/common/hashing.hpp"
#include "core/data/istream.hpp"
#include "core/data/ostream.hpp"
#include "core/io/assert.hpp"
#include "core/io/log.hpp"
#include "core/reflection/reflection_registry.hpp"
#include "core/serialization/compression.hpp"
#include "core/serialization/serialization.hpp"

namespace sfg
{
#define MESH_DEF_BLOB_MAGIC make_resource_wire_magic('M', 'D', 'E', 'F')

	bool mesh_cooker::serialize_def_blob(const mesh_def_t& def, ostream_t& stream)
	{
		stream << static_cast<u32>(MESH_DEF_BLOB_MAGIC);
		stream << static_cast<u32>(BLOB_WIRE_VERSION);

		if (!reflection_registry_t::get().type_to_stream(type_id_t<mesh_def_t>::value, const_cast<mesh_def_t*>(&def), nullptr, stream))
		{
			SFG_ERR("failed to serialize mesh definition blob");
			return false;
		}

		const u32 preview_material_count = static_cast<u32>(def.preview_materials.size());

		stream << preview_material_count;

		for (const resource_handle_t material : def.preview_materials)
			stream << material;

		return true;
	}

	bool mesh_cooker::deserialize_def_blob(istream_t& stream, mesh_def_t& out)
	{
		u32 magic	= 0;
		u32 version = 0;

		stream >> magic >> version;

		SFG_ASSERT(magic == MESH_DEF_BLOB_MAGIC);
		SFG_ASSERT(version == BLOB_WIRE_VERSION);

		if (!reflection_registry_t::get().type_from_stream(type_id_t<mesh_def_t>::value, &out, nullptr, stream))
			return false;

		if (stream.get_size() - stream.tellg() < sizeof(u32))
		{
			SFG_ERR("mesh definition blob preview materials are missing");
			return false;
		}

		u32 preview_material_count = 0;

		stream >> preview_material_count;

		if (static_cast<size_t>(preview_material_count) > (stream.get_size() - stream.tellg()) / sizeof(resource_handle_t))
		{
			SFG_ERR("mesh definition blob preview materials are invalid");
			return false;
		}

		out.preview_materials.resize(preview_material_count);

		for (resource_handle_t& material : out.preview_materials)
			stream >> material;

		return true;
	}

	bool mesh_cooker::cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		istream_t mesh_def_stream = serializer_t::load_from_file_compressed(full_path);

		if (mesh_def_stream.empty())
		{
			SFG_ERR("failed to read mesh definition file: {0}", full_path);
			return false;
		}

		mesh_def_t def = {};

		if (!deserialize_def_blob(mesh_def_stream, def))
		{
			SFG_ERR("failed to deserialize mesh definition file: {0}", full_path);
			return false;
		}

		return cook_from_def(def, out_header, stream);
	}

	bool mesh_cooker::cook_from_def(const mesh_def_t& def, resource_header_t& out_header, ostream_t& stream, bool compress)
	{
		if (def.static_primitives.empty() == def.skinned_primitives.empty())
		{
			SFG_ERR("mesh must contain either static or skinned primitives");
			return false;
		}

		if (std::any_of(def.static_primitives.begin(), def.static_primitives.end(), [](const primitive_static_def_t& primitive) { return primitive.vertices.empty() || primitive.indices.empty(); }) ||
			std::any_of(def.skinned_primitives.begin(), def.skinned_primitives.end(), [](const primitive_skinned_def_t& primitive) { return primitive.vertices.empty() || primitive.indices.empty(); }))
		{
			SFG_ERR("mesh primitive has no geometry");
			return false;
		}

		ostream_t mesh_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<mesh_def_t>::value, const_cast<mesh_def_t*>(&def), nullptr, mesh_stream))
		{
			SFG_ERR("failed to serialize mesh definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::mesh,
			.magic		 = mesh_loader_t::WIRE_MAGIC,
			.version	 = mesh_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(mesh_stream.get_raw(), mesh_stream.get_size()),
		};

		if (!compress)
		{
			stream = std::move(mesh_stream);
			return true;
		}

		stream = compressor_t::compress(mesh_stream);

		if (stream.get_size() == 0)
		{
			SFG_ERR("failed to compress mesh payload");
			return false;
		}

		return true;
	}

#undef MESH_DEF_BLOB_MAGIC
}
