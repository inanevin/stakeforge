// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	struct resource_manifest_entry_t
	{
		string_t		name;
		string_t		path;
		resource_type_e type = resource_type_e::invalid;
		nlohmann::json	config;
	};

	struct resource_manifest_t
	{
		vector_t<resource_manifest_entry_t> resources;
	};
}
