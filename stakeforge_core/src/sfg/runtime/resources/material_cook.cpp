// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material.hpp"
#include "material_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool material_cooker::cook_from_def(const material_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		for (size_t i = 0; i < def.textures.size(); ++i)
		{
			if (def.textures[i].name.empty())
			{
				SFG_ERR("material texture name is empty");
				return false;
			}

			const sid_t name_hash = hashing_t::to_sid(def.textures[i].name);

			for (size_t j = 0; j < i; ++j)
			{
				if (hashing_t::to_sid(def.textures[j].name) == name_hash)
				{
					SFG_ERR("material texture name hash is duplicated: {0}", def.textures[i].name.c_str());
					return false;
				}
			}
		}

		for (size_t i = 0; i < def.samplers.size(); ++i)
		{
			if (def.samplers[i].name.empty())
			{
				SFG_ERR("material sampler name is empty");
				return false;
			}

			const sid_t name_hash = hashing_t::to_sid(def.samplers[i].name);

			for (size_t j = 0; j < i; ++j)
			{
				if (hashing_t::to_sid(def.samplers[j].name) == name_hash)
				{
					SFG_ERR("material sampler name hash is duplicated: {0}", def.samplers[i].name.c_str());
					return false;
				}
			}
		}

		for (size_t i = 0; i < def.parameters.size(); ++i)
		{
			if (def.parameters[i].name.empty())
			{
				SFG_ERR("material parameter name is empty");
				return false;
			}

			const sid_t name_hash = hashing_t::to_sid(def.parameters[i].name);

			for (size_t j = 0; j < i; ++j)
			{
				if (hashing_t::to_sid(def.parameters[j].name) == name_hash)
				{
					SFG_ERR("material parameter name hash is duplicated: {0}", def.parameters[i].name.c_str());
					return false;
				}
			}
		}

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
