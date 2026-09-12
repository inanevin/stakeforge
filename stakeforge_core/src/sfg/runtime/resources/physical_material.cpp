/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "physical_material.hpp"
#include "physical_material_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
namespace sfg
{
	bool physical_material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read physical material resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&			 mem	 = ctx.resource_manager.get_memory();
		physical_material_runtime_t* runtime = mem.get<physical_material_runtime_t>(entry.runtime);
		*runtime							 = {};

		physical_material_def_t material = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<physical_material_def_t>::value, &material, nullptr, stream))
		{
			SFG_ERR("failed to deserialize physical material definition: {0}", entry.hash);
			return false;
		}

		runtime->restitution	 = material.restitution;
		runtime->friction		 = material.friction;
		runtime->angular_damping = material.angular_damping;
		runtime->linear_damping	 = material.linear_damping;

		return true;
	}

	void physical_material_loader_t::unload(resource_entry_t&, resource_context_t&)
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
		.unload				 = physical_material_loader_t::unload,
	};
}
