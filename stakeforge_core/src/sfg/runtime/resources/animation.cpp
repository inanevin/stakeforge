// Copyright (c) 2025 Inan Evin

#include "animation.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>

#include <new>

namespace sfg
{
	bool animation_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read animation resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);

		if (payload.empty())
		{
			SFG_ERR("failed to decompress animation payload: {0}", entry.hash);
			return false;
		}

		animation_runtime_t* runtime = ctx.resource_manager.get_memory().get<animation_runtime_t>(entry.runtime);

		std::construct_at(runtime);

		if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_def_t>::value, &runtime->def, nullptr, payload))
		{
			SFG_ERR("failed to deserialize animation definition: {0}", entry.hash);
			std::destroy_at(runtime);
			return false;
		}

		runtime->duration = runtime->def.duration;
		ctx.resource_manager.get_animation_storage().add_animation(entry.hash, runtime->def);

		return true;
	}

	void animation_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		ctx.resource_manager.get_animation_storage().remove_animation(entry.hash);
		animation_runtime_t* runtime = ctx.resource_manager.get_memory().get<animation_runtime_t>(entry.runtime);
		std::destroy_at(runtime);
	}

	const resource_type_desc_t animation_resource_desc = {
		.type				 = resource_type_e::animation,
		.runtime_size		 = sizeof(animation_runtime_t),
		.runtime_alignment	 = alignof(animation_runtime_t),
		.internals_size		 = sizeof(animation_internals_t),
		.internals_alignment = alignof(animation_internals_t),
		.wire_magic			 = animation_loader_t::WIRE_MAGIC,
		.wire_version		 = animation_loader_t::WIRE_VERSION,
		.load				 = animation_loader_t::load,
		.unload				 = animation_loader_t::unload,
	};
}
