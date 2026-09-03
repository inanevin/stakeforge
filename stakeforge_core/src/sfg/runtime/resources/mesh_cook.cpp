/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "mesh_cook.hpp"

#include "mesh.hpp"
#include "mesh_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
#define MESH_DEF_BLOB_MAGIC	  make_resource_wire_magic('M', 'D', 'E', 'F')
#define MESH_DEF_BLOB_VERSION 1

	bool mesh_cooker::serialize_def_blob(const mesh_def_t& def, ostream_t& stream)
	{
		stream << static_cast<u32>(MESH_DEF_BLOB_MAGIC);
		stream << static_cast<u32>(MESH_DEF_BLOB_VERSION);

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
		if (stream.get_size() < sizeof(u32))
		{
			SFG_ERR("mesh definition blob is too small");
			return false;
		}

		u32 magic = 0;
		stream >> magic;

		if (magic != MESH_DEF_BLOB_MAGIC)
		{
			stream.seek(0);

			return reflection_registry_t::get().type_from_stream(type_id_t<mesh_def_t>::value, &out, nullptr, stream);
		}

		if (stream.get_size() < sizeof(u32) * 2)
		{
			SFG_ERR("mesh definition blob version is missing");
			return false;
		}

		u32 version = 0;
		stream >> version;

		if (version != MESH_DEF_BLOB_VERSION)
		{
			SFG_ERR("unsupported mesh definition blob version: {0}", version);
			return false;
		}

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
		ostream_t mesh_stream;
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

#undef MESH_DEF_BLOB_VERSION
#undef MESH_DEF_BLOB_MAGIC
}
