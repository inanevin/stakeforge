// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material.hpp"
#include "material_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool material_cooker::cook_from_def(const material_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		material_def_t material = def;
		SFG_ASSERT(material.textures.empty() || material.sampler != NULL_SID);
		for (material_parameter_t& parameter : material.parameters)
			parameter.values.resize(4);

		ostream_t material_stream;
		if (!reflection_registry_t::get().type_to_stream(type_id_t<material_def_t>::value, &material, nullptr, material_stream))
		{
			SFG_ERR("failed to serialize material definition");
			return false;
		}

		out_header = {
			.magic		 = material_loader_t::WIRE_MAGIC,
			.version	 = material_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size()),
		};

		u32 dep_count = 0;

		for (const resource_handle_t h : def.textures)
		{
			const resource_dependency_t dep = {
				.handle = h,
				.type	= resource_type_e::texture,
			};
			out_header.dependencies[dep_count] = dep;
			dep_count++;
		}

		const resource_dependency_t sampler = {
			.handle = def.sampler,
			.type	= resource_type_e::texture_sampler,
		};
		out_header.dependencies[dep_count] = sampler;
		out_header.dependency_count		   = ++dep_count;

		stream.write_raw(material_stream.get_raw(), material_stream.get_size());
		return true;
	}

	bool material_cooker::collect_source_tick(const material_def_t& def, u64& out)
	{
		material_def_t material = def;
		SFG_ASSERT(material.textures.empty() || material.sampler != NULL_SID);
		for (material_parameter_t& parameter : material.parameters)
			parameter.values.resize(4);

		ostream_t material_stream;
		if (!reflection_registry_t::get().type_to_stream(type_id_t<material_def_t>::value, &material, nullptr, material_stream))
		{
			SFG_ERR("failed to serialize material definition");
			return false;
		}
		out = hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size());
		return true;
	}
}
