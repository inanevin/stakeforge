// Copyright (c) 2025 Inan Evin

#include "prefab.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool prefab_loader_t::read_source(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset, chunk_handle32_t& out_source)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read prefab resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());
		string_t prefab_source = {};

		stream >> prefab_source;

		chunk_allocator_t& mem = ctx.resource_manager.get_memory();
		out_source			   = mem.allocate_text(prefab_source.c_str());

		return true;
	}

	bool prefab_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		chunk_handle32_t source = {};

		if (!read_source(entry, ctx, rfs, payload_offset, source))
			return false;

		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		prefab_internals_t* internals = mem.get<prefab_internals_t>(entry.internals);
		*internals					  = {};
		internals->source			  = source;

		return true;
	}

	void prefab_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		prefab_internals_t* internals = mem.get<prefab_internals_t>(entry.internals);
		mem.free(internals->source);
		*internals = {};
	}

	const resource_type_desc_t prefab_resource_desc = {
		.type				 = resource_type_e::prefab,
		.runtime_size		 = sizeof(prefab_runtime_t),
		.runtime_alignment	 = alignof(prefab_runtime_t),
		.internals_size		 = sizeof(prefab_internals_t),
		.internals_alignment = alignof(prefab_internals_t),
		.wire_magic			 = prefab_loader_t::WIRE_MAGIC,
		.wire_version		 = prefab_loader_t::WIRE_VERSION,
		.load				 = prefab_loader_t::load,
		.unload				 = prefab_loader_t::unload,
	};
}
