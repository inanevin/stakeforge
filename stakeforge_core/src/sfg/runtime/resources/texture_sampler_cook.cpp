// Copyright (c) 2025 Inan Evin

#include "texture_sampler_cook.hpp"

#include "texture_sampler.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool texture_sampler_cooker::cook_from_json(const nlohmann::json& json_data, ostream_t& stream)
	{
		const sampler_desc_t	sampler = json_data.get<sampler_desc_t>();
		const string_t			data	= json_data.dump();
		const resource_header_t header	= {
			 .magic		   = texture_sampler_loader_t::WIRE_MAGIC,
			 .version	   = texture_sampler_loader_t::WIRE_VERSION,
			 .source_ticks = {hashing_t::hash_u64(data.data(), data.size())},
		 };

		header.serialize(stream);
		sampler.serialize(stream);
		return true;
	}
}
