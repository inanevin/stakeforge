// Copyright (c) 2025 Inan Evin

#include "texture_sampler_cook.hpp"

#include "texture_sampler.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/descriptions_reflection.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool texture_sampler_cooker::cook_from_json(const nlohmann::json& json_data, ostream_t& stream)
	{
		sampler_desc_t sampler = {};
		if (!reflection_registry_t::get().deserialize_from_json(sampler_desc_reflection_t::TYPE_ID, &sampler, json_data))
			return false;

		const string_t			data   = json_data.dump();
		const resource_header_t header = {
			.magic		  = texture_sampler_loader_t::WIRE_MAGIC,
			.version	  = texture_sampler_loader_t::WIRE_VERSION,
			.source_ticks = {hashing_t::hash_u64(data.data(), data.size())},
		};

		header.serialize(stream);
		sampler.serialize(stream);
		return true;
	}
}
