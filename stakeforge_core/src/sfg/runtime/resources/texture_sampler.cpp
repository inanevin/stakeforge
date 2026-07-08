// Copyright (c) 2025 Inan Evin

#include "texture_sampler.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool texture_sampler_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
		{
			SFG_ERR("failed to read texture sampler resource: {0}", entry.hash);
			return false;
		}

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&			 mem	   = ctx.resource_manager.get_memory();
		texture_sampler_runtime_t*	 runtime   = mem.get<texture_sampler_runtime_t>(entry.runtime);
		texture_sampler_internals_t* internals = mem.get<texture_sampler_internals_t>(entry.internals);
		*runtime							   = {};
		*internals							   = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<sampler_desc_t>::value, &runtime->desc, nullptr, stream))
		{
			SFG_ERR("failed to deserialize texture sampler description: {0}", entry.hash);
			return false;
		}

		sampler_desc_t desc = runtime->desc;
		desc.set_name(mem.get_text(entry.debug_name));
		ctx.resource_manager.bump_render_pending(entry);
		internals->sampler = render_resources_t::get().enqueue_create_sampler(entry.hash, entry.type, desc);
		return true;
	}

	void texture_sampler_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
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
		.use_async_load		 = false,
		.use_render_pending	 = true,
		.load				 = texture_sampler_loader_t::load,
		.unload				 = texture_sampler_loader_t::unload,
	};
}
