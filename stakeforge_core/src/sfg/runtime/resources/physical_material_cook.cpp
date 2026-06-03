// Copyright (c) 2025 Inan Evin

#include "physical_material_cook.hpp"

#include "physical_material_json.hpp"
#include "physical_material_reflection.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		bool deserialize_physical_material(const nlohmann::json& json_data, physical_material_json_t& material)
		{
			if (!reflection_registry_t::get().deserialize_from_json(physical_material_reflection_t::TYPE_ID, &material, json_data))
				return false;

			material.angular_damping = json_data.value<f32>("angular_damp", material.angular_damping);
			material.linear_damping	 = json_data.value<f32>("linear_damp", material.linear_damping);
			return true;
		}

		bool cook_physical_material(const nlohmann::json& json_data, const vector_t<u64>& source_ticks, ostream_t& stream)
		{
			physical_material_json_t material = {};
			if (!deserialize_physical_material(json_data, material))
				return false;

			const resource_header_t header = {
				.magic		  = physical_material_loader_t::WIRE_MAGIC,
				.version	  = physical_material_loader_t::WIRE_VERSION,
				.source_ticks = source_ticks,
			};

			header.serialize(stream);
			material.serialize(stream);
			return true;
		}
	}

	bool physical_material_cooker::cook_from_json(const nlohmann::json& json_data, ostream_t& stream)
	{
		const string_t		data		 = json_data.dump();
		const vector_t<u64> source_ticks = {hashing_t::hash_u64(data.data(), data.size())};
		return cook_physical_material(json_data, source_ticks, stream);
	}

	bool physical_material_cooker::cook_from_file(const char* full_path, ostream_t& stream)
	{
		const string_t		 json_text = file_system_t::read_file_as_string(full_path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse physical material {0}", full_path);
			return false;
		}

		const nlohmann::json material_json = doc.value<nlohmann::json>("settings", doc);
		const vector_t<u64>	 source_ticks  = {file_system_t::get_last_modified_ticks(full_path)};
		return cook_physical_material(material_json, source_ticks, stream);
	}
}
