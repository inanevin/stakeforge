// Copyright (c) 2025 Inan Evin

#include "physical_material.hpp"

#include "physical_material_json.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>

namespace sfg
{
	bool physical_material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&			 mem	 = ctx.resource_manager.get_memory();
		physical_material_runtime_t* runtime = mem.get<physical_material_runtime_t>(entry.runtime);
		*runtime							 = {};

		istream_t stream;
		stream.open(entry.after_header_data.data, entry.after_header_data.size);

		physical_material_json_t material = {};
		material.deserialize(stream);

		runtime->restitution	 = material.restitution;
		runtime->friction		 = material.friction;
		runtime->angular_damping = material.angular_damping;
		runtime->linear_damping	 = material.linear_damping;
		return true;
	}

	create_internals_result_e physical_material_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::ready;
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
		.wire_magic			 = physical_material_loader_t::WIRE_MAGIC,
		.wire_version		 = physical_material_loader_t::WIRE_VERSION,
		.load				 = physical_material_loader_t::load,
		.create_internals	 = physical_material_loader_t::create_internals,
		.destroy_internals	 = physical_material_loader_t::destroy_internals,
	};
}
