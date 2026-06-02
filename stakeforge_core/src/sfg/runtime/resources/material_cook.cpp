// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material_json.hpp"
#include "material_json_reflection.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool material_cooker::cook_from_json(const nlohmann::json& json_data, ostream_t& stream)
	{
		const material_json_t	material = json_data.get<material_json_t>();
		const string_t			data	 = json_data.dump();
		const resource_header_t header	 = {
			  .magic		= material_loader_t::WIRE_MAGIC,
			  .version		= material_loader_t::WIRE_VERSION,
			  .source_ticks = {hashing_t::hash_u64(data.data(), data.size())},
		  };

		header.serialize(stream);
		material.serialize(stream);
		return true;
	}
}
