/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/data/span.hpp>
#include <sfg/runtime/world/ecs_table.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct ecs_query_chunk_t
	{
		const void* table_bases[ECS_INNER_JOIN_MAX_TABLES]		  = {};
		u64			table_masks[ECS_INNER_JOIN_MAX_TABLES]		  = {};
		sid_t		component_type_ids[ECS_INNER_JOIN_MAX_TABLES] = {};
		u32			component_strides[ECS_INNER_JOIN_MAX_TABLES]  = {};
		u8			table_flags[ECS_INNER_JOIN_MAX_TABLES]		  = {};
		entity_id_t entity_base									  = NULL_ENTITY_ID;
		u64			match_mask									  = 0;
		u32			table_count									  = 0;
	};

	struct ecs_query_cursor_t
	{
		ecs_component_table_ref_t table_refs[ECS_INNER_JOIN_MAX_TABLES]	 = {};
		const ecs_node_t*		  l1_nodes[ECS_INNER_JOIN_MAX_TABLES]	 = {};
		ecs_query_row_t			  current								 = {};
		entity_id_t				  entity_index							 = 0;
		u64						  pending_bits							 = 0;
		u32						  table_count							 = 0;
		u32						  table_index							 = 0;
		u32						  same_count							 = 0;
		bool					  is_required[ECS_INNER_JOIN_MAX_TABLES] = {};
		u32						  required_count						 = 0;
		bool					  done									 = true;
	};

	struct ecs_query_range_t
	{
		ecs_component_table_ref_t table_refs[ECS_INNER_JOIN_MAX_TABLES] = {};
		u32						  table_count							= 0;

		ecs_query_range_t() = default;
		ecs_query_range_t(span_t<const ecs_component_table_ref_t> in_table_refs);

		struct iterator_t
		{
			ecs_query_cursor_t cursor = {};

			iterator_t() = default;
			iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs);

			static iterator_t make_end();

			const ecs_query_row_t& operator*() const;
			const ecs_query_row_t* operator->() const;
			iterator_t&			   operator++();
			bool				   operator==(const iterator_t& other) const;
			bool				   operator!=(const iterator_t& other) const;
		};

		iterator_t begin();
		iterator_t end();
	};

	struct ecs_query_chunk_range_t
	{
		ecs_component_table_ref_t table_refs[ECS_INNER_JOIN_MAX_TABLES] = {};
		u32						  table_count							= 0;

		ecs_query_chunk_range_t() = default;
		ecs_query_chunk_range_t(span_t<const ecs_component_table_ref_t> in_table_refs);

		struct iterator_t
		{
			span_t<const ecs_component_table_ref_t> table_refs							   = {};
			const ecs_node_t*						l1_nodes[ECS_INNER_JOIN_MAX_TABLES]	   = {};
			ecs_query_chunk_t						current								   = {};
			entity_id_t								entity_index						   = 0;
			u32										table_count							   = 0;
			u32										table_index							   = 0;
			u32										same_count							   = 0;
			bool									is_required[ECS_INNER_JOIN_MAX_TABLES] = {};
			u32										required_count						   = 0;
			bool									done								   = true;

			iterator_t() = default;
			iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs);

			static iterator_t make_end();

			const ecs_query_chunk_t& operator*() const;
			const ecs_query_chunk_t* operator->() const;
			iterator_t&				 operator++();
			bool					 operator==(const iterator_t& other) const;
			bool					 operator!=(const iterator_t& other) const;

			void init(span_t<const ecs_component_table_ref_t> in_table_refs);
			bool advance();
		};

		iterator_t begin();
		iterator_t end();
	};

	class ecs_t final
	{
		friend struct ecs_query_range_t::iterator_t;
		friend struct ecs_query_chunk_range_t::iterator_t;

	public:
		using inner_join_fn = void (*)(entity_id_t id, void** components, u32 count);

		ecs_t() = delete;

		// -----------------------------------------------------------------------------
		// query
		// -----------------------------------------------------------------------------

		static void					   inner_join(span_t<const ecs_component_table_ref_t> table_refs, inner_join_fn fn);
		static ecs_query_range_t	   inner_join(span_t<const ecs_component_table_ref_t> tables);
		static ecs_query_chunk_range_t inner_join_chunks(span_t<const ecs_component_table_ref_t> tables);
		static void					   inner_join_init(ecs_query_cursor_t& cursor, span_t<const ecs_component_table_ref_t> tables);
		static bool					   inner_join_next(ecs_query_cursor_t& cursor);

	private:
		static bool		   advance_table_entity_index(const ecs_component_table_t& table, entity_id_t& index);
		static void*	   offset(void* ptr, size_t byte_offset);
		static entity_id_t align_down_to_chunk(entity_id_t value);
		static entity_id_t align_up_to_chunk(entity_id_t value);
		static u32		   popcount(u64 value);
		static u32		   countr_zero(u64 value);
	};
}
