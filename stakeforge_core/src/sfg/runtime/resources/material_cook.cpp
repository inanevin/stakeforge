// Copyright (c) 2025 Inan Evin

#include "material_cook.hpp"

#include "material.hpp"
#include "material_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool material_cooker::cook_from_def(const material_def_t& def, ostream_t& stream)
	{
		material_def_t material = def;
		SFG_ASSERT(material.textures.empty() || material.sampler != NULL_SID);
		for (material_parameter_t& parameter : material.parameters)
			parameter.values.resize(4);

		ostream_t material_stream;
		if (!reflection_registry_t::get().serialize_to_stream(type_id_t<material_def_t>::value, &material, material_stream))
			return false;

		const resource_header_t header = {
			.magic		  = material_loader_t::WIRE_MAGIC,
			.version	  = material_loader_t::WIRE_VERSION,
			.source_ticks = {hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size())},
		};

		header.serialize(stream);
		stream.write_raw(material_stream.get_raw(), material_stream.get_size());
		return true;
	}

	bool material_cooker::collect_source_ticks(const material_def_t& def, vector_t<u64>& out)
	{
		material_def_t material = def;
		SFG_ASSERT(material.textures.empty() || material.sampler != NULL_SID);
		for (material_parameter_t& parameter : material.parameters)
			parameter.values.resize(4);

		ostream_t material_stream;
		if (!reflection_registry_t::get().serialize_to_stream(type_id_t<material_def_t>::value, &material, material_stream))
			return false;
		out.push_back(hashing_t::hash_u64(material_stream.get_raw(), material_stream.get_size()));
		return true;
	}
}
