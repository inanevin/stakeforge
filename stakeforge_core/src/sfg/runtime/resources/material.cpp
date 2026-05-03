// Copyright (c) 2025 Inan Evin

#include "material.hpp"

namespace sfg
{
	bool material_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool material_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void material_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void material_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void material_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t material_resource_desc = {
		.type				 = resource_type_e::material,
		.data_size			 = sizeof(material_data_t),
		.data_alignment		 = alignof(material_data_t),
		.internals_size		 = sizeof(material_internals_t),
		.internals_alignment = alignof(material_internals_t),
		.load				 = material_load,
		.create_internals	 = material_create_internals,
		.destroy_internals	 = material_destroy_internals,
		.unload				 = material_unload,
		.unload_cpu			 = material_unload_cpu,
	};
}
