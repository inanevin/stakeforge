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
#include <sfg/io/assert.hpp>

#include <cstddef>
#include <limits>
#include <type_traits>

namespace sfg
{
	template <typename T> struct type_id_t;

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

	enum ecs_component_table_flags_e : u8
	{
		ecs_component_table_flags_excluded = 1 << 0,
		ecs_component_table_flags_optional = 1 << 1,
	};

	class ecs_component_table_t;

	struct ecs_query_row_t
	{
		void*		components[ECS_INNER_JOIN_MAX_TABLES]		  = {};
		sid_t		component_type_ids[ECS_INNER_JOIN_MAX_TABLES] = {};
		entity_id_t id											  = NULL_ENTITY_ID;
		u32			component_count								  = 0;
		u32			component_presence_mask						  = 0;

		// -----------------------------------------------------------------------------
		// row
		// -----------------------------------------------------------------------------

		u32 get_index(sid_t type) const
		{
			for (u32 i = 0; i < component_count; i++)
			{
				if (component_type_ids[i] == type)
					return i;
			}

			return ECS_INVALID_INDEX;
		}

		template <typename T> const T& get() const
		{
			const u32 idx = get_index(type_id_t<T>::value);

			SFG_ASSERT(idx < ECS_INNER_JOIN_MAX_TABLES);

			const T* ptr = reinterpret_cast<const T*>(components[idx]);

			return *ptr;
		}

		template <typename T> const T& get(u32 index) const
		{
			const T* ptr = reinterpret_cast<const T*>(components[index]);

			return *ptr;
		}

		template <typename T> T& get_mutable() const
		{
			const u32 idx = get_index(type_id_t<T>::value);

			SFG_ASSERT(idx < ECS_INNER_JOIN_MAX_TABLES);

			T* ptr = reinterpret_cast<T*>(components[idx]);

			return *ptr;
		}

		template <typename T> T& get_mutable(u32 index) const
		{
			T* ptr = reinterpret_cast<T*>(components[index]);

			return *ptr;
		}
	};
}
