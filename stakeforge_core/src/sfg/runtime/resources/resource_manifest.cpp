// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"

namespace sfg
{
	void from_json(const nlohmann::json& j, resource_manifest_entry_t& e)
	{
		e.path	 = j.value<string_t>("path", "");
		e.name	 = j.value<string_t>("name", "");
		e.type	 = j.value<resource_type_e>("type", resource_type_e::invalid);
		e.config = j.value<nlohmann::json>("config", nlohmann::json::object());
	}

	void from_json(const nlohmann::json& j, resource_manifest_t& m)
	{
		m.resources = j.value<vector_t<resource_manifest_entry_t>>("resources", {});
	}
}
