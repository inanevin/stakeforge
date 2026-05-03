// Copyright (c) 2025 Inan Evin

#include "shader.hpp"

namespace sfg
{
	bool shader_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool shader_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
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
