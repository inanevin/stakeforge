// Copyright (c) 2025 Inan Evin

#include "material.hpp"

namespace sfg
{
	bool material_loader_t::load(resource_entry_t&, resource_context_t&)
	{
		return false;
	}

	create_internals_result_e material_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::failed;
	}

	void material_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t material_resource_desc = {
		.type				 = resource_type_e::material,
		.runtime_size		 = sizeof(material_runtime_t),
		.runtime_alignment	 = alignof(material_runtime_t),
		.internals_size		 = sizeof(material_internals_t),
		.internals_alignment = alignof(material_internals_t),
		.load				 = material_loader_t::load,
		.create_internals	 = material_loader_t::create_internals,
		.destroy_internals	 = material_loader_t::destroy_internals,
	};
}
