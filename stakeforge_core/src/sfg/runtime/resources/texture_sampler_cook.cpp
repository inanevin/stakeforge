// Copyright (c) 2025 Inan Evin

#include "texture_sampler_cook.hpp"

#include "texture_sampler.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool texture_sampler_cooker::cook_from_desc(const sampler_desc_t& desc, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t sampler_stream;
		if (!reflection_registry_t::get().type_to_stream(type_id_t<sampler_desc_t>::value, const_cast<sampler_desc_t*>(&desc), nullptr, sampler_stream))
		{
			SFG_ERR("failed to serialize texture sampler description");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::texture_sampler,
			.magic		 = texture_sampler_loader_t::WIRE_MAGIC,
			.version	 = texture_sampler_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(sampler_stream.get_raw(), sampler_stream.get_size()),
		};

		stream.write_raw(sampler_stream.get_raw(), sampler_stream.get_size());
		return true;
	}
}
