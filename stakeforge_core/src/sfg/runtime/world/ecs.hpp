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

#include <sfg/data/span.hpp>
#include <sfg/runtime/world/ecs_component_type.hpp>
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

	struct ecs_query_range_t
	{
		ecs_component_table_ref_t table_refs[ECS_INNER_JOIN_MAX_TABLES] = {};
		u32						  table_count							= 0;

		ecs_query_range_t() = default;
		ecs_query_range_t(span_t<const ecs_component_table_ref_t> in_table_refs);

		struct iterator_t
		{
			span_t<const ecs_component_table_ref_t> table_refs							   = {};
			const ecs_node_t*						l1_nodes[ECS_INNER_JOIN_MAX_TABLES]	   = {};
			ecs_query_row_t							current								   = {};
			entity_id_t								entity_index						   = 0;
			u64										pending_bits						   = 0;
			u32										table_count							   = 0;
			u32										table_index							   = 0;
			u32										same_count							   = 0;
			bool									is_required[ECS_INNER_JOIN_MAX_TABLES] = {};
			u32										required_count						   = 0;
			bool									done								   = true;

			iterator_t() = default;
			iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs);

			static iterator_t make_end();

			const ecs_query_row_t& operator*() const;
			const ecs_query_row_t* operator->() const;
			iterator_t&			   operator++();
			bool				   operator==(const iterator_t& other) const;
			bool				   operator!=(const iterator_t& other) const;

			void init(span_t<const ecs_component_table_ref_t> in_table_refs);
			bool advance();
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
		// table
		// -----------------------------------------------------------------------------

		static void	 table_init(ecs_component_table_t& table, const ecs_component_type_desc_t& type_desc);
		static void	 table_uninit(ecs_component_table_t& table);
		static void	 table_clear(ecs_component_table_t& table);
		static bool	 is_table_empty(const ecs_component_table_t& table);
		static bool	 table_has(const ecs_component_table_t& table, entity_id_t id);
		static void* table_get(const ecs_component_table_t& table, entity_id_t id);
		static void* table_add(ecs_component_table_t& table, entity_id_t id);
		static void	 table_remove(ecs_component_table_t& table, entity_id_t id);

		// -----------------------------------------------------------------------------
		// query
		// -----------------------------------------------------------------------------

		static void					   inner_join(span_t<const ecs_component_table_ref_t> table_refs, inner_join_fn fn);
		static ecs_query_range_t	   inner_join(span_t<const ecs_component_table_ref_t> tables);
		static ecs_query_chunk_range_t inner_join_chunks(span_t<const ecs_component_table_ref_t> tables);

	private:
		static void		   table_calculate_indices(entity_id_t id, u32& l0_out, u32& l1_out, u32& bit_out);
		static bool		   advance_table_entity_index(const ecs_component_table_t& table, entity_id_t& index);
		static void*	   offset(void* ptr, size_t byte_offset);
		static size_t	   align_up(size_t value, size_t alignment);
		static entity_id_t align_down_to_chunk(entity_id_t value);
		static entity_id_t align_up_to_chunk(entity_id_t value);
		static u32		   popcount(u64 value);
		static u32		   countr_zero(u64 value);
	};
}
