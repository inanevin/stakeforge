// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"
#include <cstddef>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
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

	bool resource_manifest_t::load_from_file(const char* path)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("resource manifest does not exist at {0}", path);
			return false;
		}

		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);

		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse resource manifest at {0}", path);
			return false;
		}

		const nlohmann::json manifest_resources = doc.value("resources", nlohmann::json::array());

		if (!manifest_resources.is_array())
		{
			SFG_ERR("invalid resource manifest resources at {0}", path);
			return false;
		}

		vector_t<resource_manifest_entry_t> loaded_resources = {};
		loaded_resources.reserve(manifest_resources.size());

		for (const nlohmann::json& item : manifest_resources)
		{
			resource_manifest_entry_t entry = {};
			entry.config					= nlohmann::json::object();

			if (!reflection_registry_t::get().type_from_json(type_id_t<resource_manifest_entry_t>::value, &entry, nullptr, item))
			{
				SFG_ERR("invalid resource manifest entry at {0}", path);
				return false;
			}

			loaded_resources.push_back(std::move(entry));
		}

		resources = std::move(loaded_resources);
		return true;
	}

	resource_manifest_entry_reflection_t::resource_manifest_entry_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "resource_manifest_entry_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(resource_manifest_entry_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "path", .display_name = "Path", .offset = offsetof(resource_manifest_entry_t, path), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<resource_type_e>::value, .offset = offsetof(resource_manifest_entry_t, type), .size = sizeof(resource_type_e), .type = reflected_value_type_e::u8},
					{.custom_serialization = {.to_stream_fn = &json_to_stream, .to_json_fn = &json_to_json, .from_stream_fn = &json_from_stream, .from_json_fn = &json_from_json},
					 .name				   = "config",
					 .display_name		   = "Config",
					 .offset			   = offsetof(resource_manifest_entry_t, config),
					 .size				   = sizeof(nlohmann::json),
					 .flags				   = reflected_field_flag_no_ui,
					 .type				   = reflected_value_type_e::string},
				},
			.type_id   = type_id_t<resource_manifest_entry_t>::value,
			.size	   = sizeof(resource_manifest_entry_t),
			.alignment = alignof(resource_manifest_entry_t),
		});
	}
}
