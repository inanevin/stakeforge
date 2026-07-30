/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ragdoll.hpp"

#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool ragdoll_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read ragdoll resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());

		ragdoll_def_t def = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<ragdoll_def_t>::value, &def, nullptr, stream))
		{
			SFG_ERR("failed to deserialize ragdoll definition: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		ragdoll_runtime_t* runtime = mem.get<ragdoll_runtime_t>(entry.runtime);
		*runtime				   = {
			.target_skeleton   = def.target_skeleton,
			.physical_material = def.physical_material,
			.gravity_factor	   = def.gravity_factor,
			.linear_damping	   = def.linear_damping,
			.angular_damping   = def.angular_damping,
			.part_count		   = static_cast<u32>(def.parts.size()),
			.allow_sleep	   = def.allow_sleep,
		};

		if (runtime->part_count == 0)
			return true;

		runtime->parts				  = mem.allocate_bytes(sizeof(ragdoll_part_runtime_t) * runtime->part_count, alignof(ragdoll_part_runtime_t));
		ragdoll_part_runtime_t* parts = mem.get<ragdoll_part_runtime_t>(runtime->parts);

		for (u32 i = 0; i < runtime->part_count; ++i)
		{
			const ragdoll_part_def_t& source = def.parts[i];
			parts[i]						 = {
				.local_position					= source.local_position,
				.local_rotation					= source.local_rotation,
				.twist_axis						= source.twist_axis,
				.plane_axis						= source.plane_axis,
				.radius							= source.radius,
				.half_height					= source.half_height,
				.mass							= source.mass,
				.normal_half_cone_angle_degrees = source.normal_half_cone_angle_degrees,
				.plane_half_cone_angle_degrees	= source.plane_half_cone_angle_degrees,
				.twist_min_angle_degrees		= source.twist_min_angle_degrees,
				.twist_max_angle_degrees		= source.twist_max_angle_degrees,
				.max_friction_torque			= source.max_friction_torque,
				.joint_index					= source.joint_index,
				.parent_part_index				= source.parent_part_index,
			};
		}

		return true;
	}

	void ragdoll_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		ragdoll_runtime_t* runtime = mem.get<ragdoll_runtime_t>(entry.runtime);

		if (runtime->part_count != 0)
			mem.free(runtime->parts);

		*runtime = {};
	}

	const resource_type_desc_t ragdoll_resource_desc = {
		.type				 = resource_type_e::ragdoll,
		.runtime_size		 = sizeof(ragdoll_runtime_t),
		.runtime_alignment	 = alignof(ragdoll_runtime_t),
		.internals_size		 = sizeof(ragdoll_internals_t),
		.internals_alignment = alignof(ragdoll_internals_t),
		.wire_magic			 = ragdoll_loader_t::WIRE_MAGIC,
		.wire_version		 = ragdoll_loader_t::WIRE_VERSION,
		.load				 = ragdoll_loader_t::load,
		.unload				 = ragdoll_loader_t::unload,
	};
}
