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

#pragma once

#include "world_edit/editor_world_edit_context.hpp"
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/memory/chunk_handle.hpp>

namespace sfg
{
	struct editor_command_prefab_spawn_payload_t
	{
		world_handle_t					   world					  = {};
		editor_world_edit_context_handle_t previous_selection_context = {};
		resource_handle_t				   prefab					  = NULL_RESOURCE_HANDLE;
		chunk_handle32_t				   previous_selection		  = {};
		entity_id_t						   parent					  = NULL_ENTITY_ID;
		entity_id_t						   root						  = NULL_ENTITY_ID;
		entity_id_t						   previous_anchor			  = NULL_ENTITY_ID;
		u32								   previous_selection_count	  = 0;
	};

	class editor_command_prefab_spawn_t final
	{
	public:
		editor_command_prefab_spawn_t() = delete;

		static entity_id_t spawn(world_handle_t world, resource_handle_t prefab, entity_id_t parent);
	};
}
