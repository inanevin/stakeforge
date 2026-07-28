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

#include "project_package_meta.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool project_package_meta_t::serialize(ostream_t& stream) const
	{
		stream << WIRE_MAGIC;
		stream << WIRE_VERSION;

		if (!reflection_registry_t::get().type_to_stream(type_id_t<project_settings_t>::value, const_cast<project_settings_t*>(&project_settings), nullptr, stream))
			return false;

		stream << window_style;
		stream << is_fullscreen;
		stream << window_resolution.x;
		stream << window_resolution.y;

		const u32 resource_count = static_cast<u32>(resource_map.size());

		stream << resource_count;

		for (const auto& [resource_id, resource_info] : resource_map)
		{
			stream << resource_id;
			stream << static_cast<u64>(resource_info.offset);
			stream << static_cast<u64>(resource_info.size);
		}

		stream << main_world;
		stream << debug_font;

		const u32 preload_count = static_cast<u32>(preloaded_resources.size());

		stream << preload_count;

		for (sid_t resource_id : preloaded_resources)
			stream << resource_id;

		return true;
	}

	bool project_package_meta_t::deserialize(istream_t& stream)
	{
		u32 wire_magic	 = 0;
		u32 wire_version = 0;

		stream >> wire_magic;
		stream >> wire_version;

		if (wire_magic != WIRE_MAGIC || wire_version != WIRE_VERSION)
			return false;

		project_package_meta_t meta = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<project_settings_t>::value, &meta.project_settings, nullptr, stream))
			return false;

		stream >> meta.window_style;
		stream >> meta.is_fullscreen;
		stream >> meta.window_resolution.x;
		stream >> meta.window_resolution.y;

		u32 resource_count = 0;

		stream >> resource_count;

		meta.resource_map.reserve(resource_count);

		for (u32 resource_index = 0; resource_index < resource_count; ++resource_index)
		{
			sid_t resource_id = NULL_SID;
			u64	  offset	  = 0;
			u64	  size		  = 0;

			stream >> resource_id;
			stream >> offset;
			stream >> size;

			const resource_map_info_t resource_info{
				.offset = static_cast<size_t>(offset),
				.size	= static_cast<size_t>(size),
			};
			const bool inserted = meta.resource_map.emplace(resource_id, resource_info).second;

			if (!inserted)
				return false;
		}

		stream >> meta.main_world;
		stream >> meta.debug_font;

		u32 preload_count = 0;

		stream >> preload_count;

		meta.preloaded_resources.resize(preload_count);

		for (sid_t& resource_id : meta.preloaded_resources)
			stream >> resource_id;

		*this = std::move(meta);

		return true;
	}
}
