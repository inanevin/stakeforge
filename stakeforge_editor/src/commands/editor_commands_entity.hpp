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

#include <sfg/data/frame_vector.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
#define EDITOR_ENTITY_COMMAND_NAME_SIZE 64

	struct editor_command_create_entity_payload_t
	{
		world_handle_t world								 = {};
		entity_id_t	   parent								 = NULL_ENTITY_ID;
		entity_id_t	   entity								 = NULL_ENTITY_ID;
		char		   name[EDITOR_ENTITY_COMMAND_NAME_SIZE] = {};
	};

	struct editor_command_duplicate_entity_payload_t
	{
		chunk_handle32_t streams  = {};
		chunk_handle32_t sources  = {};
		chunk_handle32_t entities = {};
		world_handle_t	 world	  = {};
		u32				 count	  = 0;
	};

	struct editor_command_destroy_entity_payload_t
	{
		chunk_handle32_t streams  = {};
		chunk_handle32_t entities = {};
		world_handle_t	 world	  = {};
		u32				 count	  = 0;
	};

	class editor_commands_entity_t final
	{
	public:
		editor_commands_entity_t() = delete;

		static entity_id_t create(world_handle_t world, entity_id_t parent);
		static entity_id_t duplicate(world_handle_t world, entity_id_t entity);
		static bool		   duplicate(world_handle_t world, const frame_vector_t<entity_id_t>& entities, frame_vector_t<entity_id_t>& out_entities);
		static bool		   destroy(world_handle_t world, entity_id_t entity);
		static bool		   destroy(world_handle_t world, const frame_vector_t<entity_id_t>& entities);
	};
}
