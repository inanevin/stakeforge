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

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/ecs.hpp>

namespace sfg
{
	class ecs_helpers_t final
	{
	public:
		ecs_helpers_t() = delete;

		// -----------------------------------------------------------------------------
		// table
		// -----------------------------------------------------------------------------

		template <typename T> static void table_init(ecs_component_table_t& table)
		{
			ecs_t::table_init(table, {.type_id = T::TYPE_ID, .size = sizeof(T), .alignment = alignof(T)});
		}

		static void table_init_tag(ecs_component_table_t& table, sid_t type_id)
		{
			ecs_t::table_init(table, {.type_id = type_id, .size = 0, .alignment = 1, .flags = ecs_component_type_flags_tag});
		}

		template <typename T> static T& table_get_as(const ecs_component_table_t& table, entity_id_t id)
		{
			void* ptr = ecs_t::table_get(table, id);
			SFG_ASSERT(ptr != nullptr);
			return *reinterpret_cast<T*>(ptr);
		}

		template <typename T> static const T& table_get_as_const(const ecs_component_table_t& table, entity_id_t id)
		{
			void* ptr = ecs_t::table_get(table, id);
			SFG_ASSERT(ptr != nullptr);
			return *reinterpret_cast<const T*>(ptr);
		}

		template <typename T> static T* table_find_as(ecs_component_table_t& table, entity_id_t id)
		{
			return reinterpret_cast<T*>(ecs_t::table_get(table, id));
		}

		template <typename T> static const T* table_find_as_const(ecs_component_table_t& table, entity_id_t id)
		{
			return reinterpret_cast<const T*>(ecs_t::table_get(table, id));
		}

		template <typename T> static T& table_add_or_get_as(ecs_component_table_t& table, entity_id_t id)
		{
			const bool existed = ecs_t::table_has(table, id);
			T*		   value   = reinterpret_cast<T*>(ecs_t::table_add(table, id));
			if (!existed)
				*value = T{};
			return *value;
		}

		// -----------------------------------------------------------------------------
		// row
		// -----------------------------------------------------------------------------

		static u32 row_get_index(const ecs_query_row_t& row, sid_t type)
		{
			for (u32 i = 0; i < row.component_count; i++)
			{
				if (row.component_type_ids[i] == type)
					return i;
			}

			return ECS_INVALID_INDEX;
		}

		template <typename T> static const T& row_get(const ecs_query_row_t& row)
		{
			const u32 idx = row_get_index(row, T::TYPE_ID);
			SFG_ASSERT(idx < ECS_INNER_JOIN_MAX_TABLES);
			const T* ptr = reinterpret_cast<const T*>(row.components[idx]);
			return *ptr;
		}

		template <typename T> static const T& row_get(const ecs_query_row_t& row, u32 index)
		{
			const T* ptr = reinterpret_cast<const T*>(row.components[index]);
			return *ptr;
		}

		template <typename T> static T& row_get_mutable(const ecs_query_row_t& row)
		{
			const u32 idx = row_get_index(row, T::TYPE_ID);
			SFG_ASSERT(idx < ECS_INNER_JOIN_MAX_TABLES);
			T* ptr = reinterpret_cast<T*>(row.components[idx]);
			return *ptr;
		}

		template <typename T> static T& row_get_mutable(const ecs_query_row_t& row, u32 index)
		{
			T* ptr = reinterpret_cast<T*>(row.components[index]);
			return *ptr;
		}
	};
}
