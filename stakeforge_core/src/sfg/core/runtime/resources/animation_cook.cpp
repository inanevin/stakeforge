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

#include "core/runtime/resources/animation_cook.hpp"

#include "core/runtime/resources/animation.hpp"
#include "core/runtime/resources/animation_def.hpp"
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
#define ANIMATION_DEF_BLOB_MAGIC make_resource_wire_magic('A', 'D', 'E', 'F')

	bool animation_cooker::serialize_def_blob(const animation_def_t& def, ostream_t& stream)
	{
		stream << static_cast<u32>(ANIMATION_DEF_BLOB_MAGIC);
		stream << static_cast<u32>(BLOB_WIRE_VERSION);

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_def_t>::value, const_cast<animation_def_t*>(&def), nullptr, stream))
		{
			SFG_ERR("failed to serialize animation definition blob");
			return false;
		}

		return true;
	}

	bool animation_cooker::deserialize_def_blob(istream_t& stream, animation_def_t& out)
	{
		u32 magic	= 0;
		u32 version = 0;

		stream >> magic >> version;

		SFG_ASSERT(magic == ANIMATION_DEF_BLOB_MAGIC);
		SFG_ASSERT(version == BLOB_WIRE_VERSION);

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
		if (def.position_channels.empty() && def.rotation_channels.empty() && def.scale_channels.empty() && def.event_count == 0)
		{
			SFG_ERR("animation has no channels or events");
			return false;
		}

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

#undef ANIMATION_DEF_BLOB_MAGIC
}
