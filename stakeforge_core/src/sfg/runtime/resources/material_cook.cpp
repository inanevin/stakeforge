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
		ostream_t material_stream = {};

		material_stream << def;

		out_header = {
			.type		 = resource_type_e::material,
			.magic		 = material_loader_t::WIRE_MAGIC,
			.version	 = material_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size()),
		};

		for (const material_texture_value_t& texture : def.textures)
		{
			if (texture.texture == NULL_SID)
				continue;

			const resource_dependency_t dependency = {
				.handle = texture.texture,
				.type	= texture.type == shader_texture_type_e::texture_cube ? resource_type_e::cubemap : resource_type_e::texture,
			};

			stream << dependency;
			++out_header.dependency_count;
		}

		for (const material_sampler_value_t& sampler : def.samplers)
		{
			if (sampler.sampler == NULL_SID)
				continue;

			const resource_dependency_t dependency = {
				.handle = sampler.sampler,
				.type	= resource_type_e::texture_sampler,
			};

			stream << dependency;
			++out_header.dependency_count;
		}

		if (def.shader != NULL_SID)
		{
			const resource_dependency_t dependency = {
				.handle = def.shader,
				.type	= resource_type_e::shader,
			};

			stream << dependency;
			++out_header.dependency_count;
		}

		stream.write_raw(material_stream.get_raw(), material_stream.get_size());

		return true;
	}

	bool material_cooker::collect_source_tick(const material_def_t& def, u64& out)
	{
		ostream_t material_stream = {};
		material_stream << def;
		out = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size());
		return true;
	}
}
