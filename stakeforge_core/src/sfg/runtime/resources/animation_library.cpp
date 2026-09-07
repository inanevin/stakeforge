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

#include "animation_library.hpp"
#include "animation_library_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool animation_library_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read animation library resource: {0}", entry.hash);
			return false;
		}

		animation_library_def_t def = {};
		istream_t				stream(file_stream.get_raw(), file_stream.get_size());

		if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_library_def_t>::value, &def, nullptr, stream))
		{
			SFG_ERR("failed to deserialize animation library definition: {0}", entry.hash);
			return false;
		}

		animation_library_runtime_t& runtime = *ctx.resource_manager.get_memory().get<animation_library_runtime_t>(entry.runtime);

		runtime = {.skeleton = def.skeleton};

		return true;
	}

	void animation_library_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		*ctx.resource_manager.get_memory().get<animation_library_runtime_t>(entry.runtime) = {};
	}

	const resource_type_desc_t animation_library_resource_desc = {
		.type				 = resource_type_e::animation_library,
		.runtime_size		 = sizeof(animation_library_runtime_t),
		.runtime_alignment	 = alignof(animation_library_runtime_t),
		.internals_size		 = sizeof(animation_library_internals_t),
		.internals_alignment = alignof(animation_library_internals_t),
		.wire_magic			 = animation_library_loader_t::WIRE_MAGIC,
		.wire_version		 = animation_library_loader_t::WIRE_VERSION,
		.load				 = animation_library_loader_t::load,
		.unload				 = animation_library_loader_t::unload,
	};
}
