// Copyright (c) 2025 Inan Evin

#include "skeleton.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool skeleton_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
			return false;

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&	mem		= ctx.resource_manager.get_memory();
		skeleton_runtime_t* runtime = mem.get<skeleton_runtime_t>(entry.runtime);
		*runtime					= {};

		skeleton_def_t skeleton = {};
		if (!reflection_registry_t::get().deserialize_from_stream(type_id_t<skeleton_def_t>::value, &skeleton, stream))
			return false;

		const u32 joint_count = static_cast<u32>(skeleton.joints.size());
		SFG_ASSERT(joint_count <= MAX_JOINTS);

		runtime->joint_count	  = joint_count;
		runtime->root_joint_index = skeleton.root_joint_index;
		for (u32 i = 0; i < joint_count; ++i)
		{
			const skeleton_joint_def_t& joint = skeleton.joints[i];
			runtime->joints[i]				  = {
							   .inverse_bind = joint.inverse_bind,
							   .name_hash	 = joint.name_hash,
							   .parent_index = joint.parent_index,
			   };
		}

		return true;
	}

	void skeleton_loader_t::unload(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t skeleton_resource_desc = {
		.type				 = resource_type_e::skeleton,
		.runtime_size		 = sizeof(skeleton_runtime_t),
		.runtime_alignment	 = alignof(skeleton_runtime_t),
		.internals_size		 = sizeof(skeleton_internals_t),
		.internals_alignment = alignof(skeleton_internals_t),
		.wire_magic			 = skeleton_loader_t::WIRE_MAGIC,
		.wire_version		 = skeleton_loader_t::WIRE_VERSION,
		.use_async_load		 = false,
		.load				 = skeleton_loader_t::load,
		.unload				 = skeleton_loader_t::unload,
	};
}
