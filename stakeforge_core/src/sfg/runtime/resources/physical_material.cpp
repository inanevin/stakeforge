// Copyright (c) 2025 Inan Evin

#include "physical_material.hpp"

namespace sfg
{
	bool physical_material_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e physical_material_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void physical_material_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t physical_material_resource_desc = {
		.type				 = resource_type_e::physical_material,
		.runtime_size		 = sizeof(physical_material_runtime_t),
		.runtime_alignment	 = alignof(physical_material_runtime_t),
		.internals_size		 = sizeof(physical_material_internals_t),
		.internals_alignment = alignof(physical_material_internals_t),
		.load				 = physical_material_loader_t::load,
		.create_internals	 = physical_material_loader_t::create_internals,
		.destroy_internals	 = physical_material_loader_t::destroy_internals,
	};
}
