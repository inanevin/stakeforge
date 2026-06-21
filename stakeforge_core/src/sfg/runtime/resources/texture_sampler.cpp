// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"

#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool texture_sampler_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		   mem	   = ctx.resource_manager.get_memory();
		texture_sampler_runtime_t* runtime = mem.get<texture_sampler_runtime_t>(entry.runtime);
		*runtime						   = {};

		istream_t stream;
		stream.open(entry.load_data.data, entry.load_data.size);
		return reflection_registry_t::get().deserialize_from_stream(type_id_t<sampler_desc_t>::value, &runtime->desc, stream);
	}

	create_internals_result_e texture_sampler_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&				 mem	   = ctx.resource_manager.get_memory();
		const texture_sampler_runtime_t* runtime   = mem.get<texture_sampler_runtime_t>(entry.runtime);
		texture_sampler_internals_t*	 internals = mem.get<texture_sampler_internals_t>(entry.internals);

		*internals = {};

		sampler_desc_t desc = runtime->desc;
		desc.set_name(mem.get_text(entry.debug_name));
		render_resources_t::get().enqueue_create_sampler(entry.hash, entry.type, desc);
		return create_internals_result_e::queued;
	}

	resource_ready_result_e texture_sampler_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t&			 mem	   = ctx.resource_manager.get_memory();
		texture_sampler_internals_t* internals = mem.get<texture_sampler_internals_t>(entry.internals);

		SFG_ASSERT(completion.kind == render_resource_kind_e::sampler);

		if (completion.state == resource_state_e::failed)
			return resource_ready_result_e::failed;

		internals->sampler	 = completion.sampler;
		internals->gpu_index = completion.gpu_index;
		return resource_ready_result_e::ready;
	}

	void texture_sampler_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&			 mem	   = ctx.resource_manager.get_memory();
		texture_sampler_internals_t* internals = mem.get<texture_sampler_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_sampler(internals->sampler);
		*internals = {};
	}

	const resource_type_desc_t texture_sampler_resource_desc = {
		.type				 = resource_type_e::texture_sampler,
		.runtime_size		 = sizeof(texture_sampler_runtime_t),
		.runtime_alignment	 = alignof(texture_sampler_runtime_t),
		.internals_size		 = sizeof(texture_sampler_internals_t),
		.internals_alignment = alignof(texture_sampler_internals_t),
		.wire_magic			 = texture_sampler_loader_t::WIRE_MAGIC,
		.wire_version		 = texture_sampler_loader_t::WIRE_VERSION,
		.load				 = texture_sampler_loader_t::load,
		.create_internals	 = texture_sampler_loader_t::create_internals,
		.resource_ready		 = texture_sampler_loader_t::resource_ready,
		.destroy_internals	 = texture_sampler_loader_t::destroy_internals,
	};
}
