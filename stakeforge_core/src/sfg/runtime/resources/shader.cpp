// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool shader_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx)
	{
		u32 magic		 = 0;
		u32 version		 = 0;
		u32 payload_size = 0;
		stream >> magic >> version >> payload_size;
		if (magic != shader_wire_magic || version != shader_wire_version)
		{
			SFG_ERR("invalid shader binary, magic={0} version={1}", magic, version);
			return false;
		}

		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		shader_data_t*	   data = mem.get<shader_data_t>(entry.cpu_data);
		*data					= shader_data_t{};

		data->blobs		 = entry.payload;
		data->blobs_size = payload_size;

		stream >> data->type >> data->compile_variant_count >> data->pso_variant_count;

		SFG_ASSERT(data->compile_variant_count <= shader_max_compile_variants);
		SFG_ASSERT(data->pso_variant_count <= shader_max_pso_variants);

		for (u8 i = 0; i < data->compile_variant_count; ++i)
		{
			shader_compile_variant_t& cv = data->compile_variants[i];
			stream >> cv.stage_count;
			SFG_ASSERT(cv.stage_count <= shader_max_stages_per_variant);
			for (u8 j = 0; j < cv.stage_count; ++j)
			{
				shader_stage_entry_t& s = cv.stages[j];
				stream >> s.stage >> s.offset >> s.size;
			}
		}

		for (u8 i = 0; i < data->pso_variant_count; ++i)
		{
			shader_pso_variant_t& pv = data->pso_variants[i];
			stream >> pv.compile_variant_index >> pv.variant_flags;
		}

		if (payload_size != 0)
		{
			u8* dst = mem.get(entry.payload.head);
			stream.read_to_raw(dst, static_cast<size_t>(payload_size));
		}

		return true;
	}

	bool shader_create_internals(resource_entry_t&, resource_context_t&)
	{
		return true;
	}

	void shader_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void shader_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void shader_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t shader_resource_desc = {
		.type				 = resource_type_e::shader,
		.data_size			 = sizeof(shader_data_t),
		.data_alignment		 = alignof(shader_data_t),
		.internals_size		 = sizeof(shader_internals_t),
		.internals_alignment = alignof(shader_internals_t),
		.load				 = shader_load,
		.create_internals	 = shader_create_internals,
		.destroy_internals	 = shader_destroy_internals,
		.unload				 = shader_unload,
		.unload_cpu			 = shader_unload_cpu,
	};
}
