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

#include "physical_material.hpp"
#include "physical_material_def.hpp"
#include "physical_material_def.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/reflection/reflection_registry.hpp>
namespace sfg
{
	bool physical_material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, istream_t& stream)
	{
		chunk_allocator_t&			 mem	 = ctx.resource_manager.get_memory();
		physical_material_runtime_t* runtime = mem.get<physical_material_runtime_t>(entry.runtime);
		*runtime							 = {};

		physical_material_def_t material = {};
		reflection_registry_t::get().deserialize_from_stream(type_id_t<physical_material_def_t>::value, &material, stream);

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
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.use_async_load		 = false,
		.load				 = physical_material_loader_t::load,
		.unload				 = physical_material_loader_t::unload,
	};
}
