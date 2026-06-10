// Copyright (c) 2025 Inan Evin

#include "animation.hpp"

#include "resource_manager.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <new>

namespace sfg
{
	bool animation_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		animation_runtime_t* runtime = ctx.resource_manager.get_memory().get<animation_runtime_t>(entry.runtime);
		std::construct_at(runtime);

		istream_t stream;
		stream.open(entry.after_header_data.data, entry.after_header_data.size);

		if (!reflection_registry_t::get().deserialize_from_stream(type_id_t<animation_def_t>::value, &runtime->def, stream))
		{
			std::destroy_at(runtime);
			return false;
		}

		runtime->duration = runtime->def.duration;
		return true;
	}

	create_internals_result_e animation_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		animation_runtime_t* runtime = ctx.resource_manager.get_memory().get<animation_runtime_t>(entry.runtime);
		ctx.resource_manager.get_animation_storage().add_animation(entry.hash, runtime->def);
		return create_internals_result_e::ready;
	}

	void animation_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
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
		.create_internals	 = animation_loader_t::create_internals,
		.destroy_internals	 = animation_loader_t::destroy_internals,
	};
}
