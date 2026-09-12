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

#include "ecs.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	ecs_query_range_t::ecs_query_range_t(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		SFG_ASSERT(in_table_refs.size <= ECS_INNER_JOIN_MAX_TABLES);

		if (in_table_refs.size > ECS_INNER_JOIN_MAX_TABLES)
			return;

		table_count = static_cast<u32>(in_table_refs.size);

		for (u32 i = 0; i < table_count; ++i)
			table_refs[i] = in_table_refs.data[i];
	}

	ecs_query_range_t::iterator_t::iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		ecs_t::inner_join_init(cursor, in_table_refs);
		ecs_t::inner_join_next(cursor);
	}

	ecs_query_range_t::iterator_t ecs_query_range_t::iterator_t::make_end()
	{
		iterator_t it  = {};
		it.cursor.done = true;
		return it;
	}

	const ecs_query_row_t& ecs_query_range_t::iterator_t::operator*() const
	{
		return cursor.current;
	}

	const ecs_query_row_t* ecs_query_range_t::iterator_t::operator->() const
	{
		return &cursor.current;
	}

	ecs_query_range_t::iterator_t& ecs_query_range_t::iterator_t::operator++()
	{
		ecs_t::inner_join_next(cursor);
		return *this;
	}

	bool ecs_query_range_t::iterator_t::operator==(const iterator_t& other) const
	{
		return cursor.done == other.cursor.done;
	}

	bool ecs_query_range_t::iterator_t::operator!=(const iterator_t& other) const
	{
		return !(*this == other);
	}

	void ecs_t::inner_join_init(ecs_query_cursor_t& cursor, span_t<const ecs_component_table_ref_t> tables)
	{
		cursor			   = {};
		cursor.table_count = static_cast<u32>(tables.size);

		SFG_ASSERT(cursor.table_count != 0);
		SFG_ASSERT(cursor.table_count <= ECS_INNER_JOIN_MAX_TABLES);

		if (cursor.table_count == 0 || cursor.table_count > ECS_INNER_JOIN_MAX_TABLES)
			return;

		for (u32 i = 0; i < cursor.table_count; ++i)
		{
			cursor.table_refs[i] = tables.data[i];

			SFG_ASSERT(cursor.table_refs[i].table != nullptr);

			if (cursor.table_refs[i].table == nullptr)
				return;

			const bool optional = (cursor.table_refs[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (cursor.table_refs[i].flags & ecs_component_table_flags_excluded) != 0;

			cursor.is_required[i] = !optional && !excluded;

			if (cursor.is_required[i])
				cursor.required_count++;
		}

		SFG_ASSERT(cursor.required_count != 0);

		if (cursor.required_count == 0)
			return;

		cursor.current.component_count = cursor.table_count;

		for (u32 i = 0; i < cursor.table_count; ++i)
			cursor.current.component_type_ids[i] = cursor.table_refs[i].table->_component_type_id;

		cursor.done = false;
	}

	bool ecs_t::inner_join_next(ecs_query_cursor_t& cursor)
	{
		if (cursor.done)
			return false;

		cursor.current.component_count = cursor.table_count;

		while (true)
		{
			if (cursor.pending_bits != 0)
			{
				const u32 bit = countr_zero(cursor.pending_bits);
				cursor.pending_bits &= cursor.pending_bits - 1;
				cursor.current.id					   = cursor.entity_index + bit;
				cursor.current.component_presence_mask = 0;

				for (u32 i = 0; i < cursor.table_count; ++i)
				{
					const bool		  excluded = (cursor.table_refs[i].flags & ecs_component_table_flags_excluded) != 0;
					const ecs_node_t* l1_node  = cursor.l1_nodes[i];

					if (excluded || l1_node == nullptr)
					{
						cursor.current.components[i] = nullptr;
						continue;
					}

					const u64 bitmask = 1ull << bit;

					if ((l1_node->mask & bitmask) == 0)
					{
						cursor.current.components[i] = nullptr;
						continue;
					}

					cursor.current.component_presence_mask |= 1u << i;

					if (cursor.table_refs[i].table->_component_struct_stride == 0)
					{
						cursor.current.components[i] = nullptr;
						continue;
					}

					const u32 prefix			 = popcount(l1_node->mask & (bitmask - 1ull));
					cursor.current.components[i] = offset(l1_node->child, prefix * cursor.table_refs[i].table->_component_struct_stride);
				}

				if (cursor.pending_bits == 0)
				{
					cursor.entity_index += ECS_L1_SPAN;
					cursor.same_count = 0;
				}

				return true;
			}

			if (cursor.entity_index >= ECS_MAX_ENTITIES)
			{
				cursor.done = true;
				return false;
			}

			for (u32 k = 0; k < cursor.table_count; ++k)
			{
				if (cursor.is_required[cursor.table_index])
					break;

				cursor.table_index = (cursor.table_index + 1) % cursor.table_count;
			}

			const entity_id_t previous_entity = cursor.entity_index;

			if (!advance_table_entity_index(*cursor.table_refs[cursor.table_index].table, cursor.entity_index))
			{
				cursor.done = true;
				return false;
			}

			if (previous_entity == cursor.entity_index)
				cursor.same_count++;
			else
				cursor.same_count = 1;

			cursor.table_index = (cursor.table_index + 1) % cursor.table_count;

			if (cursor.same_count < cursor.required_count)
				continue;

			u32 l0	= 0;
			u32 l1	= 0;
			u32 bit = 0;
			ecs_component_table_t::calculate_indices(cursor.entity_index, l0, l1, bit);

			u64 include_mask = ~0ull;
			u64 exclude_mask = 0ull;

			for (u32 i = 0; i < cursor.table_count; ++i)
			{
				const ecs_node_t* l0_node = cursor.table_refs[i].table->_l0_nodes + l0;
				const ecs_node_t* l1_node = l0_node->child == nullptr ? nullptr : reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;

				cursor.l1_nodes[i] = l1_node;

				const bool optional = (cursor.table_refs[i].flags & ecs_component_table_flags_optional) != 0;
				const bool excluded = (cursor.table_refs[i].flags & ecs_component_table_flags_excluded) != 0;
				const bool required = !optional && !excluded;

				if (required)
				{
					if (l1_node == nullptr)
					{
						include_mask = 0;
						break;
					}

					include_mask &= l1_node->mask;

					if (include_mask == 0)
						break;
				}
				else if (excluded && l1_node != nullptr)
				{
					exclude_mask |= l1_node->mask;
				}
			}

			cursor.pending_bits = include_mask & ~exclude_mask;

			if (cursor.pending_bits == 0)
			{
				cursor.entity_index += ECS_L1_SPAN;
				cursor.same_count = 0;
				continue;
			}
		}
	}

	ecs_query_range_t::iterator_t ecs_query_range_t::begin()
	{
		return iterator_t{{.data = table_refs, .size = table_count}};
	}

	ecs_query_range_t::iterator_t ecs_query_range_t::end()
	{
		return iterator_t::make_end();
	}

	ecs_query_chunk_range_t::ecs_query_chunk_range_t(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		SFG_ASSERT(in_table_refs.size <= ECS_INNER_JOIN_MAX_TABLES);

		if (in_table_refs.size > ECS_INNER_JOIN_MAX_TABLES)
			return;

		table_count = static_cast<u32>(in_table_refs.size);

		for (u32 i = 0; i < table_count; ++i)
			table_refs[i] = in_table_refs.data[i];
	}

	ecs_query_chunk_range_t::iterator_t::iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		init(in_table_refs);
	}

	ecs_query_chunk_range_t::iterator_t ecs_query_chunk_range_t::iterator_t::make_end()
	{
		iterator_t it = {};
		it.done		  = true;
		return it;
	}

	const ecs_query_chunk_t& ecs_query_chunk_range_t::iterator_t::operator*() const
	{
		return current;
	}

	const ecs_query_chunk_t* ecs_query_chunk_range_t::iterator_t::operator->() const
	{
		return &current;
	}

	ecs_query_chunk_range_t::iterator_t& ecs_query_chunk_range_t::iterator_t::operator++()
	{
		if (!advance())
			done = true;
		return *this;
	}

	bool ecs_query_chunk_range_t::iterator_t::operator==(const iterator_t& other) const
	{
		return done == other.done;
	}

	bool ecs_query_chunk_range_t::iterator_t::operator!=(const iterator_t& other) const
	{
		return !(*this == other);
	}

	void ecs_query_chunk_range_t::iterator_t::init(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		table_refs	= in_table_refs;
		table_count = static_cast<u32>(table_refs.size);

		done		 = true;
		entity_index = 0;
		table_index	 = 0;
		same_count	 = 0;
		current		 = {};

		SFG_ASSERT(table_count != 0);
		SFG_ASSERT(table_count <= ECS_INNER_JOIN_MAX_TABLES);

		if (table_count == 0 || table_count > ECS_INNER_JOIN_MAX_TABLES)
			return;

		required_count = 0;

		for (u32 i = 0; i < table_count; ++i)
		{
			SFG_ASSERT(table_refs.data[i].table != nullptr);

			if (table_refs.data[i].table == nullptr)
				return;

			const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;

			is_required[i] = !optional && !excluded;

			if (is_required[i])
				required_count++;
		}

		SFG_ASSERT(required_count != 0);

		if (required_count == 0)
			return;

		current.table_count = table_count;

		for (u32 i = 0; i < table_count; ++i)
		{
			current.component_type_ids[i] = table_refs.data[i].table->_component_type_id;
			current.component_strides[i]  = static_cast<u32>(table_refs.data[i].table->_component_struct_stride);
			current.table_flags[i]		  = table_refs.data[i].flags;
		}

		done = false;
	}

	bool ecs_query_chunk_range_t::iterator_t::advance()
	{
		if (done)
			return false;

		current.table_count = table_count;

		while (true)
		{
			if (entity_index >= ECS_MAX_ENTITIES)
				return false;

			for (u32 k = 0; k < table_count; ++k)
			{
				if (is_required[table_index])
					break;
				table_index = (table_index + 1) % table_count;
			}

			const entity_id_t prev = entity_index;

			if (!ecs_t::advance_table_entity_index(*table_refs.data[table_index].table, entity_index))
				return false;

			if (prev == entity_index)
				same_count++;
			else
				same_count = 1;

			table_index = (table_index + 1) % table_count;

			if (same_count < required_count)
				continue;

			u32 l0	= 0;
			u32 l1	= 0;
			u32 bit = 0;
			ecs_component_table_t::calculate_indices(entity_index, l0, l1, bit);

			u64 include_mask = ~0ull;
			u64 exclude_mask = 0ull;

			for (u32 i = 0; i < table_count; ++i)
			{
				const ecs_node_t* l0_node = table_refs.data[i].table->_l0_nodes + l0;
				const ecs_node_t* l1_node = l0_node->child == nullptr ? nullptr : reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;

				l1_nodes[i]			   = l1_node;
				current.table_masks[i] = l1_node != nullptr ? l1_node->mask : 0;
				current.table_bases[i] = l1_node != nullptr ? l1_node->child : nullptr;

				const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
				const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
				const bool required = !optional && !excluded;

				if (required)
				{
					if (l1_node == nullptr)
					{
						include_mask = 0;
						break;
					}

					include_mask &= l1_node->mask;

					if (include_mask == 0)
						break;
				}
				else if (excluded && l1_node != nullptr)
				{
					exclude_mask |= l1_node->mask;
				}
			}

			const u64 match_bits = include_mask & ~exclude_mask;

			if (match_bits == 0)
			{
				entity_index += ECS_L1_SPAN;
				same_count = 0;
				continue;
			}

			current.entity_base = entity_index;
			current.match_mask	= match_bits;

			entity_index += ECS_L1_SPAN;
			same_count = 0;
			return true;
		}
	}

	ecs_query_chunk_range_t::iterator_t ecs_query_chunk_range_t::begin()
	{
		iterator_t it{{.data = table_refs, .size = table_count}};

		if (!it.advance())
			return iterator_t::make_end();
		return it;
	}

	ecs_query_chunk_range_t::iterator_t ecs_query_chunk_range_t::end()
	{
		return iterator_t::make_end();
	}

	void ecs_t::inner_join(span_t<const ecs_component_table_ref_t> table_refs, inner_join_fn fn)
	{
		SFG_ASSERT(table_refs.size != 0);
		SFG_ASSERT(table_refs.size <= ECS_INNER_JOIN_MAX_TABLES);
		SFG_ASSERT(fn != nullptr);

		if (table_refs.size == 0 || table_refs.size > ECS_INNER_JOIN_MAX_TABLES || fn == nullptr)
			return;

		u32		  required_count						 = 0;
		bool	  is_required[ECS_INNER_JOIN_MAX_TABLES] = {};
		const u32 table_count							 = static_cast<u32>(table_refs.size);

		for (u32 i = 0; i < table_count; ++i)
		{
			SFG_ASSERT(table_refs.data[i].table != nullptr);

			if (table_refs.data[i].table == nullptr)
				return;

			const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;

			is_required[i] = !optional && !excluded;

			if (is_required[i])
				required_count++;
		}

		SFG_ASSERT(required_count != 0);

		if (required_count == 0)
			return;

		void*		row_ptrs[ECS_INNER_JOIN_MAX_TABLES] = {};
		entity_id_t entity_index						= 0;
		u32			same_count							= 0;
		u32			table_index							= 0;

		while (entity_index < ECS_MAX_ENTITIES)
		{
			for (u32 k = 0; k < table_count; ++k)
			{
				if (is_required[table_index])
					break;

				table_index = (table_index + 1) % table_count;
			}

			const entity_id_t prev = entity_index;

			if (!advance_table_entity_index(*table_refs.data[table_index].table, entity_index))
				break;

			if (prev == entity_index)
				same_count++;
			else
				same_count = 1;

			table_index = (table_index + 1) % table_count;

			if (same_count < required_count)
				continue;

			u32 l0	= 0;
			u32 l1	= 0;
			u32 bit = 0;
			ecs_component_table_t::calculate_indices(entity_index, l0, l1, bit);

			const ecs_node_t* l1_nodes[ECS_INNER_JOIN_MAX_TABLES] = {};
			u64				  include_mask						  = ~0ull;
			u64				  exclude_mask						  = 0ull;

			for (u32 i = 0; i < table_count; i++)
			{
				const ecs_node_t* l0_node = table_refs.data[i].table->_l0_nodes + l0;
				const ecs_node_t* l1_node = l0_node->child == nullptr ? nullptr : reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;

				l1_nodes[i] = l1_node;

				const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
				const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
				const bool required = !optional && !excluded;

				if (required)
				{
					if (l1_node == nullptr)
					{
						include_mask = 0;
						break;
					}

					include_mask &= l1_node->mask;

					if (include_mask == 0)
						break;
				}
				else if (excluded && l1_node != nullptr)
				{
					exclude_mask |= l1_node->mask;
				}
			}

			u64 bits = include_mask & ~exclude_mask;

			while (bits != 0)
			{
				const u32 bit_index = countr_zero(bits);
				bits &= bits - 1;

				for (u32 i = 0; i < table_count; i++)
				{
					const bool		  excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
					const ecs_node_t* l1_node  = l1_nodes[i];

					if (excluded || l1_node == nullptr)
					{
						row_ptrs[i] = nullptr;
						continue;
					}

					const u64 bitmask = 1ull << bit_index;

					if (table_refs.data[i].table->_component_struct_stride == 0 || (l1_node->mask & bitmask) == 0)
					{
						row_ptrs[i] = nullptr;
						continue;
					}

					const u32 prefix = popcount(l1_node->mask & (bitmask - 1ull));
					row_ptrs[i]		 = offset(l1_node->child, prefix * table_refs.data[i].table->_component_struct_stride);
				}

				fn(entity_index + bit_index, row_ptrs, table_count);
			}

			entity_index += ECS_L1_SPAN;
			same_count = 0;
		}
	}

	ecs_query_range_t ecs_t::inner_join(span_t<const ecs_component_table_ref_t> tables)
	{
		return ecs_query_range_t{tables};
	}

	ecs_query_chunk_range_t ecs_t::inner_join_chunks(span_t<const ecs_component_table_ref_t> tables)
	{
		return ecs_query_chunk_range_t{tables};
	}

	bool ecs_t::advance_table_entity_index(const ecs_component_table_t& table, entity_id_t& index)
	{
		index = align_down_to_chunk(index);

		while (index < ECS_MAX_ENTITIES)
		{
			u32 l0	= 0;
			u32 l1	= 0;
			u32 bit = 0;
			ecs_component_table_t::calculate_indices(index, l0, l1, bit);

			const ecs_node_t* l0_node = table._l0_nodes + l0;
			const u64		  m0	  = l0_node->mask;

			if (m0 == 0)
			{
				index = align_up_to_chunk((l0 + 1u) * ECS_L0_SPAN);
				continue;
			}

			const u64 keep = ~0ull << l1;
			const u64 cand = m0 & keep;

			if (cand == 0)
			{
				index = align_up_to_chunk((l0 + 1u) * ECS_L0_SPAN);
				continue;
			}

			const u32 next_l1 = countr_zero(cand);
			index			  = l0 * ECS_L0_SPAN + next_l1 * ECS_L1_SPAN;
			return true;
		}

		return false;
	}

	void* ecs_t::offset(void* ptr, size_t byte_offset)
	{
		return reinterpret_cast<u8*>(ptr) + byte_offset;
	}

	entity_id_t ecs_t::align_down_to_chunk(entity_id_t value)
	{
		return value & ~(ECS_L1_SPAN - 1u);
	}

	entity_id_t ecs_t::align_up_to_chunk(entity_id_t value)
	{
		return (value + ECS_L1_SPAN - 1u) & ~(ECS_L1_SPAN - 1u);
	}

	u32 ecs_t::popcount(u64 value)
	{
		return static_cast<u32>(std::popcount(value));
	}

	u32 ecs_t::countr_zero(u64 value)
	{
		return static_cast<u32>(std::countr_zero(value));
	}
}
