// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material.hpp"
#include "material_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>

namespace sfg
{
	bool material_cooker::cook_from_def(const material_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t material_stream;
		material_stream << def;

		out_header = {
			.type		 = resource_type_e::material,
			.magic		 = material_loader_t::WIRE_MAGIC,
			.version	 = material_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size()),
		};

		out_header.dependency_count = 0;

		for (const material_texture_value_t& texture : def.textures)
		{
			if (texture.texture == NULL_SID)
				continue;

			out_header.dependencies[out_header.dependency_count] = {
				.handle = texture.texture,
				.type	= resource_type_e::texture,
			};
			out_header.dependency_count++;
		}

		for (const material_sampler_value_t& sampler : def.samplers)
		{
			if (sampler.sampler == NULL_SID)
				continue;

			out_header.dependencies[out_header.dependency_count] = {
				.handle = sampler.sampler,
				.type	= resource_type_e::texture_sampler,
			};
			out_header.dependency_count++;
		}

		if (def.shader != NULL_SID)
		{
			out_header.dependencies[out_header.dependency_count] = {
				.handle = def.shader,
				.type	= resource_type_e::shader,
			};
			out_header.dependency_count++;
		}

		stream.write_raw(material_stream.get_raw(), material_stream.get_size());
		return true;
	}

	bool material_cooker::collect_source_tick(const material_def_t& def, u64& out)
	{
		ostream_t material_stream;
		material_stream << def;
		out = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size());
		return true;
	}
}
