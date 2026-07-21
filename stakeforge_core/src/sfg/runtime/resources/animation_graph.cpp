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

#include "animation_graph.hpp"

#include <sfg/io/log.hpp>

namespace sfg
{
	bool animation_graph_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		SFG_ERR("animation graph loading is not implemented");

		return false;
	}

	void animation_graph_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
	}

	const resource_type_desc_t animation_graph_resource_desc = {
		.type				 = resource_type_e::animation_graph,
		.runtime_size		 = sizeof(animation_graph_runtime_t),
		.runtime_alignment	 = alignof(animation_graph_runtime_t),
		.internals_size		 = sizeof(animation_graph_internals_t),
		.internals_alignment = alignof(animation_graph_internals_t),
		.load				 = animation_graph_loader_t::load,
		.unload				 = animation_graph_loader_t::unload,
	};
}
