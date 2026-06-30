// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"
#include <cstddef>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	namespace
	{
		void json_to_stream(void* obj, void*, ostream_t& out_stream)
		{
			const nlohmann::json& value = *static_cast<nlohmann::json*>(obj);
			out_stream << string_t(value.dump().c_str());
		}

		void json_to_json(void* obj, void*, nlohmann::json& out_json)
		{
			out_json = *static_cast<nlohmann::json*>(obj);
		}

		void json_from_stream(void* obj, void*, istream_t& in_stream)
		{
			string_t value;
			in_stream >> value;

			nlohmann::json& config = *static_cast<nlohmann::json*>(obj);
			config				   = nlohmann::json::parse(value, nullptr, false);
			if (config.is_discarded())
				config = nlohmann::json::object();
		}

		void json_from_json(void* obj, void*, const nlohmann::json& in_json)
		{
			*static_cast<nlohmann::json*>(obj) = in_json;
		}
	}

	resource_manifest_entry_reflection_t::resource_manifest_entry_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "resource_manifest_entry_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(resource_manifest_entry_t, name), .size = sizeof(string_t), .type = reflected_value_type_e_v2::string},
					{.name = "path", .display_name = "Path", .offset = offsetof(resource_manifest_entry_t, path), .size = sizeof(string_t), .type = reflected_value_type_e_v2::string},
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<resource_type_e>::value, .offset = offsetof(resource_manifest_entry_t, type), .size = sizeof(resource_type_e), .type = reflected_value_type_e_v2::u8},
					{.custom_serialization = {.to_stream_fn = &json_to_stream, .to_json_fn = &json_to_json, .from_stream_fn = &json_from_stream, .from_json_fn = &json_from_json},
					 .name				   = "config",
					 .display_name		   = "Config",
					 .offset			   = offsetof(resource_manifest_entry_t, config),
					 .size				   = sizeof(nlohmann::json),
					 .flags				   = reflected_field_flag_no_ui,
					 .type				   = reflected_value_type_e_v2::string},
				},
			.type_id   = type_id_t<resource_manifest_entry_t>::value,
			.size	   = sizeof(resource_manifest_entry_t),
			.alignment = alignof(resource_manifest_entry_t),
		});
	}
}
