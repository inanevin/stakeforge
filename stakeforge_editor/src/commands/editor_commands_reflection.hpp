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

#include "editor_command_system.hpp"
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class ostream_t;

	enum class editor_reflected_edit_target_kind_e : u8
	{
		none,
		raw_object,
		world_component,
		world_components,
	};

	struct editor_reflected_edit_target_t
	{
		void*								object			  = nullptr;
		const entity_id_t*					entities		  = nullptr;
		editor_command_listener_handle_t	required_listener = {};
		world_handle_t						world			  = {};
		entity_id_t							entity			  = NULL_ENTITY_ID;
		sid_t								type_id			  = 0;
		u32									entity_count	  = 0;
		editor_reflected_edit_target_kind_e kind			  = editor_reflected_edit_target_kind_e::none;
	};

	struct editor_command_reflected_field_edit_payload_t
	{
		chunk_handle32_t			   old_value	= {};
		chunk_handle32_t			   new_value	= {};
		chunk_handle32_t			   old_values	= {};
		chunk_handle32_t			   entities		= {};
		editor_reflected_edit_target_t target		= {};
		world_handle_t				   world		= {};
		sid_t						   type_id		= 0;
		sid_t						   field_id		= 0;
		u32							   entity_count = 0;
		bool						   text_id		= false;
	};

	struct editor_reflected_field_edit_desc_t
	{
		editor_reflected_edit_target_t target	= {};
		sid_t						   type_id	= 0;
		sid_t						   field_id = 0;
	};

	class editor_commands_reflection_t final
	{
	public:
		editor_commands_reflection_t() = delete;

		static bool													edit_field(const editor_reflected_field_edit_desc_t& desc, const ostream_t& old_value, const ostream_t& new_value);
		static bool													edit_text_id_field(const editor_reflected_field_edit_desc_t& desc, world_handle_t world, const char* old_value, const char* new_value);
		static const editor_command_reflected_field_edit_payload_t* get_payload(const editor_command_system_t& system, const editor_command_t& command);
	};
}
