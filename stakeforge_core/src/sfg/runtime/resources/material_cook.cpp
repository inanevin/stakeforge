// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material_def.hpp"
#include "material_def_reflection.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void normalize_material_parameters(material_def_t& material)
		{
			for (material_parameter_t& parameter : material.parameters)
				parameter.values.resize(4);
		}
	}

	bool material_cooker::cook_from_json(const nlohmann::json& json_data, ostream_t& stream)
	{
		material_def_t material = {};
		if (!reflection_registry_t::get().deserialize_from_json(material_def_reflection_t::TYPE_ID, &material, json_data))
			return false;
		SFG_ASSERT(material.textures.empty() || material.sampler != NULL_SID);
		normalize_material_parameters(material);

		const string_t			data   = json_data.dump();
		const resource_header_t header = {
			.magic		  = material_loader_t::WIRE_MAGIC,
			.version	  = material_loader_t::WIRE_VERSION,
			.source_ticks = {hashing_t::hash_u64(data.data(), data.size())},
		};

		header.serialize(stream);
		return reflection_registry_t::get().serialize_to_stream(material_def_reflection_t::TYPE_ID, &material, stream);
	}
}
