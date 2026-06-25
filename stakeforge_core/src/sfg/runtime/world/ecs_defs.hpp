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

#include <sfg/common/size_definitions.hpp>

#include <cstddef>
#include <limits>
#include <type_traits>

namespace sfg
{
	using entity_id_t	= u32;
	using entity_guid_t = u64;

#define NULL_ENTITY_ID	 std::numeric_limits<entity_id_t>::max()
#define NULL_ENTITY_GUID std::numeric_limits<entity_guid_t>::max()
	static inline constexpr entity_id_t ECS_MAX_ENTITIES		  = 256000;
	static inline constexpr u32			ECS_L0_SPAN				  = 4096;
	static inline constexpr u32			ECS_L1_SPAN				  = 64;
	static inline constexpr u32			ECS_L0_SIZE				  = (ECS_MAX_ENTITIES + ECS_L0_SPAN - 1) / ECS_L0_SPAN;
	static inline constexpr u32			ECS_INNER_JOIN_MAX_TABLES = 16;
	static inline constexpr u32			ECS_INVALID_INDEX		  = UINT32_MAX;

	struct ecs_node_t
	{
		u64	  mask;
		void* child;
	};

	static_assert(std::is_trivial_v<ecs_node_t>);
	static_assert(alignof(ecs_node_t) == 8);

	enum ecs_component_table_flags_e : u8
	{
		ecs_component_table_flags_excluded = 1 << 0,
		ecs_component_table_flags_optional = 1 << 1,
	};

	struct ecs_component_table_t;

	struct ecs_component_table_ref_t
	{
		const ecs_component_table_t* table = nullptr;
		u8							 flags = 0;

		ecs_component_table_ref_t optional() const
		{
			return {.table = table, .flags = static_cast<u8>(flags | ecs_component_table_flags_optional)};
		}

		ecs_component_table_ref_t excluded() const
		{
			return {.table = table, .flags = static_cast<u8>(flags | ecs_component_table_flags_excluded)};
		}

		ecs_component_table_ref_t operator!() const
		{
			return excluded();
		}
	};

	struct ecs_component_table_t
	{
		ecs_node_t* l0_nodes				   = nullptr;
		size_t		component_struct_stride	   = 0;
		size_t		component_struct_alignment = 0;
		sid_t		component_type_id		   = 0;

		ecs_component_table_ref_t ref() const
		{
			return {.table = this, .flags = 0};
		}
	};

	struct ecs_query_row_t
	{
		void*		components[ECS_INNER_JOIN_MAX_TABLES]		  = {};
		sid_t		component_type_ids[ECS_INNER_JOIN_MAX_TABLES] = {};
		entity_id_t id											  = NULL_ENTITY_ID;
		u32			component_count								  = 0;
	};
}
