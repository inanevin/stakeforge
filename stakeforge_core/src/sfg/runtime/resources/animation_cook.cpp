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

#include "animation_cook.hpp"

#include "animation.hpp"
#include "animation_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
#define ANIMATION_DEF_BLOB_MAGIC   make_resource_wire_magic('A', 'D', 'E', 'F')
#define ANIMATION_DEF_BLOB_VERSION 2

	bool animation_cooker::serialize_def_blob(const animation_def_t& def, ostream_t& stream)
	{
		stream << static_cast<u32>(ANIMATION_DEF_BLOB_MAGIC);
		stream << static_cast<u32>(ANIMATION_DEF_BLOB_VERSION);

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_def_t>::value, const_cast<animation_def_t*>(&def), nullptr, stream))
		{
			SFG_ERR("failed to serialize animation definition blob");
			return false;
		}

		return true;
	}

	bool animation_cooker::deserialize_def_blob(istream_t& stream, animation_def_t& out)
	{
		if (stream.get_size() < sizeof(u32))
		{
			SFG_ERR("animation definition blob is too small");
			return false;
		}

		u32 magic = 0;
		stream >> magic;

		if (magic != ANIMATION_DEF_BLOB_MAGIC)
		{
			stream.seek(0);

			reflection_registry_t& registry = reflection_registry_t::get();
			const sid_t			   type_id	= type_id_t<animation_def_t>::value;

			registry.type_field_from_stream(type_id, TO_SID("name"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("name_hash"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("duration"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("preview_mesh"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("preview_skeleton"), &out, nullptr, stream);

			if (stream.get_size() - stream.tellg() < sizeof(u32))
			{
				SFG_ERR("legacy animation definition preview materials are missing");
				return false;
			}

			u32 preview_material_count = 0;
			stream >> preview_material_count;

			const size_t preview_material_bytes = static_cast<size_t>(preview_material_count) * sizeof(resource_handle_t);

			if (preview_material_bytes > stream.get_size() - stream.tellg())
			{
				SFG_ERR("legacy animation definition preview materials are invalid");
				return false;
			}

			stream.skip_by(preview_material_bytes);

			registry.type_field_from_stream(type_id, TO_SID("position_channels"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("rotation_channels"), &out, nullptr, stream);
			registry.type_field_from_stream(type_id, TO_SID("scale_channels"), &out, nullptr, stream);

			return true;
		}

		if (stream.get_size() < sizeof(u32) * 2)
		{
			SFG_ERR("animation definition blob version is missing");
			return false;
		}

		u32 version = 0;
		stream >> version;

		if (version != ANIMATION_DEF_BLOB_VERSION && version != 1)
		{
			SFG_ERR("unsupported animation definition blob version: {0}", version);
			return false;
		}

		if (version == 1)
		{
			const sid_t fields[] = {TO_SID("name"), TO_SID("name_hash"), TO_SID("duration"), TO_SID("preview_mesh"), TO_SID("preview_skeleton"), TO_SID("position_channels"), TO_SID("rotation_channels"), TO_SID("scale_channels")};

			for (const sid_t field : fields)
			{
				reflection_registry_t::get().type_field_from_stream(type_id_t<animation_def_t>::value, field, &out, nullptr, stream);
			}

			return true;
		}

		return reflection_registry_t::get().type_from_stream(type_id_t<animation_def_t>::value, &out, nullptr, stream);
	}

	bool animation_cooker::cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		istream_t animation_def_stream = serializer_t::load_from_file_compressed(full_path);

		if (animation_def_stream.empty())
		{
			SFG_ERR("failed to read animation definition file: {0}", full_path);
			return false;
		}

		animation_def_t def = {};

		if (!deserialize_def_blob(animation_def_stream, def))
		{
			SFG_ERR("failed to deserialize animation definition file: {0}", full_path);
			return false;
		}

		return cook_from_def(def, out_header, stream);
	}

	bool animation_cooker::cook_from_def(const animation_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t animation_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_def_t>::value, const_cast<animation_def_t*>(&def), nullptr, animation_stream))
		{
			SFG_ERR("failed to serialize animation definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::animation,
			.magic		 = animation_loader_t::WIRE_MAGIC,
			.version	 = animation_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(animation_stream.get_raw(), animation_stream.get_size()),
		};

		stream = compressor_t::compress(animation_stream);
		if (stream.get_size() == 0)
		{
			SFG_ERR("failed to compress animation payload");
			return false;
		}

		return true;
	}

#undef ANIMATION_DEF_BLOB_VERSION
#undef ANIMATION_DEF_BLOB_MAGIC
}
