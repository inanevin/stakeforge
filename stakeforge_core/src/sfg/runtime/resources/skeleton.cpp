// Copyright (c) 2025 Inan Evin

#include "skeleton.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool skeleton_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read skeleton resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&	mem		= ctx.resource_manager.get_memory();
		skeleton_runtime_t* runtime = mem.get<skeleton_runtime_t>(entry.runtime);
		*runtime					= {};

		skeleton_def_t skeleton = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<skeleton_def_t>::value, &skeleton, nullptr, stream))
		{
			SFG_ERR("failed to deserialize skeleton definition: {0}", entry.hash);
			return false;
		}

		const u32 joint_count = static_cast<u32>(skeleton.joints.size());
		SFG_ASSERT(joint_count != 0);

		runtime->joint_count	  = joint_count;
		runtime->slot_count		  = static_cast<u32>(skeleton.slots.size());
		runtime->root_joint_index = skeleton.root_joint_index;
		runtime->joints			  = mem.allocate_bytes(sizeof(skeleton_joint_runtime_t) * joint_count, alignof(skeleton_joint_runtime_t));
		runtime->evaluation_order = mem.allocate<u32>(joint_count);

		if (runtime->slot_count != 0)
			runtime->slots = mem.allocate_bytes(sizeof(skeleton_slot_runtime_t) * runtime->slot_count, alignof(skeleton_slot_runtime_t));

		skeleton_joint_runtime_t* runtime_joints		   = mem.get<skeleton_joint_runtime_t>(runtime->joints);
		u32*					  runtime_evaluation_order = mem.get<u32>(runtime->evaluation_order);

		for (u32 i = 0; i < joint_count; ++i)
		{
			const skeleton_joint_def_t& joint = skeleton.joints[i];
			const chunk_handle32_t		name  = mem.allocate_text(joint.name.c_str());

			runtime_joints[i] = {
				.local		  = joint.local,
				.inverse_bind = joint.inverse_bind,
				.name_hash	  = joint.name_hash,
				.name		  = name,
				.parent_index = joint.parent_index,
			};

			runtime_evaluation_order[i] = skeleton.evaluation_order[i];
		}

		if (runtime->slot_count != 0)
		{
			skeleton_slot_runtime_t* runtime_slots = mem.get<skeleton_slot_runtime_t>(runtime->slots);

			for (u32 i = 0; i < runtime->slot_count; ++i)
			{
				const skeleton_slot_def_t& slot		  = skeleton.slots[i];
				const chunk_handle32_t	   debug_name = mem.allocate_text(slot.slot_name);

				runtime_slots[i] = {
					.slot_name_hash	  = TO_SID(slot.slot_name),
					.debug_name		  = debug_name,
					.local_rotation	  = slot.local_rotation,
					.local_position	  = slot.local_position,
					.slot_joint_index = slot.slot_joint_index,
				};
			}
		}

		return true;
	}

	void skeleton_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		  mem	  = ctx.resource_manager.get_memory();
		skeleton_runtime_t*		  runtime = mem.get<skeleton_runtime_t>(entry.runtime);
		skeleton_joint_runtime_t* joints  = mem.get<skeleton_joint_runtime_t>(runtime->joints);

		for (u32 i = 0; i < runtime->joint_count; ++i)
			mem.free(joints[i].name);

		mem.free(runtime->joints);
		mem.free(runtime->evaluation_order);

		if (runtime->slot_count != 0)
		{
			skeleton_slot_runtime_t* slots = mem.get<skeleton_slot_runtime_t>(runtime->slots);

			for (u32 i = 0; i < runtime->slot_count; ++i)
				mem.free(slots[i].debug_name);

			mem.free(runtime->slots);
		}

		*runtime = {};
	}

	const resource_type_desc_t skeleton_resource_desc = {
		.type				 = resource_type_e::skeleton,
		.runtime_size		 = sizeof(skeleton_runtime_t),
		.runtime_alignment	 = alignof(skeleton_runtime_t),
		.internals_size		 = sizeof(skeleton_internals_t),
		.internals_alignment = alignof(skeleton_internals_t),
		.wire_magic			 = skeleton_loader_t::WIRE_MAGIC,
		.wire_version		 = skeleton_loader_t::WIRE_VERSION,
		.load				 = skeleton_loader_t::load,
		.unload				 = skeleton_loader_t::unload,
	};
}
