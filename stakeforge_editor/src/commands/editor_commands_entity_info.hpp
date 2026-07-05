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
#include <sfg/data/span.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

#define EDITOR_ENTITY_INFO_NAME_SIZE 256

	struct editor_entity_info_data_t
	{
		char	name[EDITOR_ENTITY_INFO_NAME_SIZE] = {};
		vec3f_t pos								   = vec3f_t::zero;
		quat_t	rot								   = {};
		vec3f_t scale							   = vec3f_t::one;
	};

	struct editor_command_paste_entity_info_payload_t
	{
		chunk_handle32_t		  old_infos = {};
		chunk_handle32_t		  entities	= {};
		editor_entity_info_data_t info		= {};
		world_handle_t			  world		= {};
		u32						  count		= 0;
	};

	struct editor_command_edit_entity_info_payload_t
	{
		chunk_handle32_t previous_infos = {};
		chunk_handle32_t post_infos		= {};
		chunk_handle32_t entities		= {};
		world_handle_t	 world			= {};
		u32				 count			= 0;
	};

	class editor_commands_entity_info_t final
	{
	public:
		editor_commands_entity_info_t() = delete;

		static editor_entity_info_data_t read(world_t& world, entity_id_t entity);
		static void						 apply(world_t& world, entity_id_t entity, const editor_entity_info_data_t& info);
		static bool						 paste(world_handle_t world, entity_id_t entity, const editor_entity_info_data_t& info);
		static bool						 paste(world_handle_t world, const frame_vector_t<entity_id_t>& entities, const editor_entity_info_data_t& info);
		static bool						 edit(world_handle_t world, span_t<const entity_id_t> entities, span_t<const editor_entity_info_data_t> previous_infos, span_t<const editor_entity_info_data_t> post_infos);
	};
}
