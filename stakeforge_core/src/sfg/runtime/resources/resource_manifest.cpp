// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	resource_manifest_entry_reflection_t::resource_manifest_entry_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<resource_manifest_entry_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "name", .display_name = "Name", .type = reflected_value_type_e::string, .offset = offsetof(resource_manifest_entry_t, name), .size = sizeof(string_t)},
			{.name = "path", .display_name = "Path", .type = reflected_value_type_e::string, .offset = offsetof(resource_manifest_entry_t, path), .size = sizeof(string_t)},
			{.name = "type", .display_name = "Type", .type = reflected_value_type_e::enum8, .sub_type_id = type_id_t<resource_type_e>::value, .offset = offsetof(resource_manifest_entry_t, type), .size = sizeof(resource_type_e)},
			{.name = "config", .display_name = "Config", .type = reflected_value_type_e::json, .offset = offsetof(resource_manifest_entry_t, config), .size = sizeof(nlohmann::json), .flags = reflected_field_flags_no_ui},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "resource_manifest_entry_t",
			.type_id   = type_id_t<resource_manifest_entry_t>::value,
			.size	   = sizeof(resource_manifest_entry_t),
			.alignment = alignof(resource_manifest_entry_t),
		});
	}
}
