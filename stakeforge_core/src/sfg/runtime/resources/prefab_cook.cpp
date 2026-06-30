// Copyright (c) 2025 Inan Evin

#include "prefab_cook.hpp"
#include "world_cook_entity_header.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{

	bool prefab_cooker::cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		const string_t		 text = file_system_t::read_file_as_string(full_path);
		const nlohmann::json json = nlohmann::json::parse(text, nullptr, false);
		if (json.is_discarded())
		{
			SFG_ERR("failed to parse prefab json: {0}", full_path);
			return false;
		}

		return cook_from_json(json, out_header, stream);
	}

	bool prefab_cooker::cook_from_json(const nlohmann::json& json, resource_header_t& out_header, ostream_t& stream)
	{
		

		return true;
	}
}
