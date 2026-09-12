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
		stream << script_assembly_name;

		const u32 resource_count = static_cast<u32>(resource_map.size());

		stream << resource_count;

		for (const auto& [resource_id, resource_info] : resource_map)
		{
			stream << resource_id;
			stream << static_cast<u64>(resource_info.offset);
			stream << static_cast<u64>(resource_info.size);
		}

		const u32 world_count = static_cast<u32>(worlds.size());

		stream << world_count;

		for (const world_meta_t& world : worlds)
		{
			stream << world.sid;
			stream << world.name_hash;
		}

		stream << main_world.sid;
		stream << main_world.name_hash;

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
		stream >> meta.script_assembly_name;

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

		u32 world_count = 0;

		stream >> world_count;

		meta.worlds.resize(world_count);

		for (world_meta_t& world : meta.worlds)
		{
			stream >> world.sid;
			stream >> world.name_hash;
		}

		stream >> meta.main_world.sid;
		stream >> meta.main_world.name_hash;

		*this = std::move(meta);

		return true;
	}
}
