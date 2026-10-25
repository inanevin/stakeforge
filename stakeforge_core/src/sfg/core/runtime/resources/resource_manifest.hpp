// Copyright (c) 2025 Inan Evin
#pragma once

#include "core/common/sfg_exports.hpp"

#include "core/common/type_id.hpp"

#include "core/runtime/resources/common_resources.hpp"
#include "core/data/string.hpp"
#include "core/data/vector.hpp"
#include "core/vendor/nhlohmann/json.hpp"

namespace sfg
{
	struct resource_manifest_entry_t
	{
		string_t		name;
		string_t		path;
		resource_type_e type = resource_type_e::invalid;
		nlohmann::json	config;
	};

	struct SFG_API resource_manifest_t
	{
		vector_t<resource_manifest_entry_t> resources;

		bool load_from_file(const char* path);
	};

	SFG_DEFINE_TYPE_ID(resource_manifest_entry_t);

	struct resource_manifest_entry_reflection_t
	{
		resource_manifest_entry_reflection_t();
	};

}
