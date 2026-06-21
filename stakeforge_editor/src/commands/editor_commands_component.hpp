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
	struct editor_command_add_component_payload_t
	{
		chunk_handle32_t entities		= {};
		world_handle_t	 world			= {};
		sid_t			 component_type = 0;
		u32				 count			= 0;
	};

	struct editor_command_remove_component_payload_t
	{
		chunk_handle32_t streams		= {};
		chunk_handle32_t entities		= {};
		world_handle_t	 world			= {};
		sid_t			 component_type = 0;
		u32				 count			= 0;
	};

	struct editor_command_reset_component_payload_t
	{
		chunk_handle32_t streams		= {};
		chunk_handle32_t entities		= {};
		world_handle_t	 world			= {};
		sid_t			 component_type = 0;
		u32				 count			= 0;
	};

	struct editor_command_paste_component_payload_t
	{
		chunk_handle32_t old_streams	= {};
		chunk_handle32_t paste_stream	= {};
		chunk_handle32_t entities		= {};
		world_handle_t	 world			= {};
		sid_t			 component_type = 0;
		u32				 count			= 0;
	};

	class editor_commands_component_t final
	{
	public:
		editor_commands_component_t() = delete;

		static bool add(world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool add(world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool remove(world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool remove(world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool reset(world_handle_t world, entity_id_t entity, sid_t component_type);
		static bool reset(world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type);
		static bool paste(world_handle_t world, entity_id_t entity, sid_t component_type, const u8* data, size_t data_size);
		static bool paste(world_handle_t world, const frame_vector_t<entity_id_t>& entities, sid_t component_type, const u8* data, size_t data_size);
	};
}
