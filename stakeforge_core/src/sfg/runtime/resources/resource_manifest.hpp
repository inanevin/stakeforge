// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct resource_manifest_entry_t
	{
		string_t		name;
		string_t		path;
		resource_type_e type = resource_type_e::invalid;
	};

	struct resource_manifest_t
	{
		vector_t<resource_manifest_entry_t> resources;
	};

	void to_json(nlohmann::json& j, const resource_type_e& t);
	void from_json(const nlohmann::json& j, resource_type_e& t);

	void to_json(nlohmann::json& j, const resource_manifest_entry_t& e);
	void from_json(const nlohmann::json& j, resource_manifest_entry_t& e);

	void to_json(nlohmann::json& j, const resource_manifest_t& m);
	void from_json(const nlohmann::json& j, resource_manifest_t& m);
}
