// Copyright (c) 2025 Inan Evin

#include "physical_material.hpp"

namespace sfg
{
	bool physical_material_load(resource_entry_t&, istream_t&, resource_context_t&)
	{
		return false;
	}

	bool physical_material_create_internals(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	void physical_material_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void physical_material_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void physical_material_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t physical_material_resource_desc = {
		.type				 = resource_type_e::physical_material,
		.data_size			 = sizeof(physical_material_data_t),
		.data_alignment		 = alignof(physical_material_data_t),
		.internals_size		 = sizeof(physical_material_internals_t),
		.internals_alignment = alignof(physical_material_internals_t),
		.load				 = physical_material_load,
		.create_internals	 = physical_material_create_internals,
		.destroy_internals	 = physical_material_destroy_internals,
		.unload				 = physical_material_unload,
		.unload_cpu			 = physical_material_unload_cpu,
	};
}
