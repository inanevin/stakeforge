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
		if (in_table_refs.size > ECS_INNER_JOIN_MAX_TABLES)
			return;

		table_count = static_cast<u32>(in_table_refs.size);
		for (u32 i = 0; i < table_count; ++i)
			table_refs[i] = in_table_refs.data[i];
	}

	ecs_query_range_t::iterator_t::iterator_t(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		init(in_table_refs);
		if (!advance())
			done = true;
	}

	ecs_query_range_t::iterator_t ecs_query_range_t::iterator_t::make_end()
	{
		iterator_t it;
		it.done = true;
		return it;
	}

	const ecs_query_row_t& ecs_query_range_t::iterator_t::operator*() const
	{
		return current;
	}

	const ecs_query_row_t* ecs_query_range_t::iterator_t::operator->() const
	{
		return &current;
	}

	ecs_query_range_t::iterator_t& ecs_query_range_t::iterator_t::operator++()
	{
		if (!advance())
			done = true;
		return *this;
	}

	bool ecs_query_range_t::iterator_t::operator==(const iterator_t& other) const
	{
		return done == other.done;
	}

	bool ecs_query_range_t::iterator_t::operator!=(const iterator_t& other) const
	{
		return !(*this == other);
	}

	void ecs_query_range_t::iterator_t::init(span_t<const ecs_component_table_ref_t> in_table_refs)
	{
		table_refs	= in_table_refs;
		table_count = static_cast<u32>(table_refs.size);

		done		 = true;
		pending_bits = 0;
		entity_index = 0;
		table_index	 = 0;
		same_count	 = 0;
		if (table_count == 0 || table_count > ECS_INNER_JOIN_MAX_TABLES)
			return;

		required_count = 0;
		for (u32 i = 0; i < table_count; ++i)
		{
			if (table_refs.data[i].table == nullptr)
				return;

			const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
			is_required[i]		= !optional && !excluded;
			if (is_required[i])
				required_count++;
		}

		if (required_count == 0)
			return;

		current.component_count = table_count;
		for (u32 i = 0; i < table_count; ++i)
			current.component_type_ids[i] = table_refs.data[i].table->component_type_id;
		done = false;
	}

	bool ecs_query_range_t::iterator_t::advance()
	{
		if (done)
			return false;

		current.component_count = table_count;

		while (true)
		{
			if (pending_bits != 0)
			{
				const u32 bit = ecs_t::countr_zero(pending_bits);
				pending_bits &= pending_bits - 1;
				current.id = entity_index + bit;

				for (u32 i = 0; i < table_count; ++i)
				{
					const bool		  excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
					const ecs_node_t* l1_node  = l1_nodes[i];

					if (excluded || l1_node == nullptr)
					{
						current.components[i] = nullptr;
						continue;
					}

					const u64 bitmask = 1ull << bit;
					if (table_refs.data[i].table->component_struct_stride == 0 || (l1_node->mask & bitmask) == 0)
					{
						current.components[i] = nullptr;
						continue;
					}

					const u32 prefix	  = ecs_t::popcount(l1_node->mask & (bitmask - 1ull));
					current.components[i] = ecs_t::offset(l1_node->child, prefix * table_refs.data[i].table->component_struct_stride);
				}

				if (pending_bits == 0)
				{
					entity_index += ECS_L1_SPAN;
					same_count = 0;
				}
				return true;
			}

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
			ecs_t::table_calculate_indices(entity_index, l0, l1, bit);

			u64 include_mask = ~0ull;
			u64 exclude_mask = 0ull;

			for (u32 i = 0; i < table_count; ++i)
			{
				const ecs_node_t* l0_node = table_refs.data[i].table->l0_nodes + l0;
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

			pending_bits = include_mask & ~exclude_mask;

			if (pending_bits == 0)
			{
				entity_index += ECS_L1_SPAN;
				same_count = 0;
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
		iterator_t it;
		it.done = true;
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
		if (table_count == 0 || table_count > ECS_INNER_JOIN_MAX_TABLES)
			return;

		required_count = 0;
		for (u32 i = 0; i < table_count; ++i)
		{
			if (table_refs.data[i].table == nullptr)
				return;

			const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;
			is_required[i]		= !optional && !excluded;
			if (is_required[i])
				required_count++;
		}

		if (required_count == 0)
			return;

		current.table_count = table_count;
		for (u32 i = 0; i < table_count; ++i)
		{
			current.component_type_ids[i] = table_refs.data[i].table->component_type_id;
			current.component_strides[i]  = static_cast<u32>(table_refs.data[i].table->component_struct_stride);
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
			ecs_t::table_calculate_indices(entity_index, l0, l1, bit);

			u64 include_mask = ~0ull;
			u64 exclude_mask = 0ull;

			for (u32 i = 0; i < table_count; ++i)
			{
				const ecs_node_t* l0_node = table_refs.data[i].table->l0_nodes + l0;
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

	void ecs_t::table_init(ecs_component_table_t& table, const ecs_component_type_desc_t& type_desc)
	{
		SFG_ASSERT(table.l0_nodes == nullptr);
		SFG_ASSERT(type_desc.type_id != 0);
		SFG_ASSERT(type_desc.alignment != 0);

		table.l0_nodes = reinterpret_cast<ecs_node_t*>(SFG_ALIGNED_MALLOC(alignof(ecs_node_t), sizeof(ecs_node_t) * ECS_L0_SIZE));
		SFG_MEMSET(table.l0_nodes, 0, sizeof(ecs_node_t) * ECS_L0_SIZE);
		table.type_desc					 = type_desc;
		table.component_type_id			 = type_desc.type_id;
		table.component_struct_alignment = type_desc.alignment;
		table.component_struct_stride	 = align_up(type_desc.size, type_desc.alignment);
	}

	void ecs_t::table_uninit(ecs_component_table_t& table)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);

		for (u32 i = 0; i < ECS_L0_SIZE; i++)
		{
			ecs_node_t* node = table.l0_nodes + i;
			if (node->mask == 0)
				continue;

			ecs_node_t* l1_nodes = reinterpret_cast<ecs_node_t*>(node->child);
			for (u32 k = 0; k < ECS_L1_SPAN; ++k)
			{
				if (l1_nodes[k].child != nullptr)
					SFG_ALIGNED_FREE(l1_nodes[k].child);
			}

			SFG_ALIGNED_FREE(node->child);
		}

		SFG_ALIGNED_FREE(table.l0_nodes);
		table = {};
	}

	void ecs_t::table_clear(ecs_component_table_t& table)
	{
		const ecs_component_type_desc_t type_desc = table.type_desc;
		table_uninit(table);
		table_init(table, type_desc);
	}

	bool ecs_t::is_table_empty(const ecs_component_table_t& table)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);

		for (u32 i = 0; i < ECS_L0_SIZE; ++i)
		{
			const ecs_node_t* l0_node = table.l0_nodes + i;
			if (l0_node->mask != 0)
				return false;
		}

		return true;
	}

	bool ecs_t::table_has(const ecs_component_table_t& table, entity_id_t id)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;
		table_calculate_indices(id, l0, l1, bit);

		const ecs_node_t* l0_node = table.l0_nodes + l0;
		if ((l0_node->mask & (1llu << l1)) == 0)
			return false;

		const ecs_node_t* l1_node = reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;
		return (l1_node->mask & (1llu << bit)) != 0;
	}

	void* ecs_t::table_get(const ecs_component_table_t& table, entity_id_t id)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		if (table.component_struct_stride == 0)
			return nullptr;

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;
		table_calculate_indices(id, l0, l1, bit);

		const ecs_node_t* l0_node = table.l0_nodes + l0;
		if ((l0_node->mask & (1llu << l1)) == 0)
			return nullptr;

		const ecs_node_t* l1_node = reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;
		const u64		  bitmask = 1ull << bit;
		if ((l1_node->mask & bitmask) == 0)
			return nullptr;

		const u32 prefix = popcount(l1_node->mask & (bitmask - 1ull));
		return offset(l1_node->child, prefix * table.component_struct_stride);
	}

	void* ecs_t::table_add(ecs_component_table_t& table, entity_id_t id)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;
		table_calculate_indices(id, l0, l1, bit);

		ecs_node_t* l0_node = table.l0_nodes + l0;
		if (l0_node->mask == 0)
		{
			const size_t alignment = std::max(alignof(ecs_node_t), static_cast<size_t>(8));
			l0_node->child		   = SFG_ALIGNED_MALLOC(alignment, sizeof(ecs_node_t) * ECS_L1_SPAN);
			SFG_MEMSET(l0_node->child, 0, sizeof(ecs_node_t) * ECS_L1_SPAN);
		}

		l0_node->mask |= 1llu << l1;

		ecs_node_t* l1_node = reinterpret_cast<ecs_node_t*>(l0_node->child) + l1;
		const u64	bitmask = 1llu << bit;

		if (table.component_struct_stride == 0)
		{
			l1_node->mask |= bitmask;
			return nullptr;
		}

		if (l1_node->mask == 0)
		{
			const size_t alignment = std::max(table.component_struct_alignment, static_cast<size_t>(8));
			l1_node->child		   = SFG_ALIGNED_MALLOC(alignment, table.component_struct_stride * ECS_L1_SPAN);
		}

		const u32 prefix = popcount(l1_node->mask & (bitmask - 1ull));
		if ((l1_node->mask & bitmask) != 0)
			return offset(l1_node->child, table.component_struct_stride * prefix);

		const u32 count = popcount(l1_node->mask);
		if (count > prefix)
		{
			void* base = l1_node->child;
			SFG_MEMMOVE(offset(base, (prefix + 1) * table.component_struct_stride), offset(base, prefix * table.component_struct_stride), (count - prefix) * table.component_struct_stride);
		}

		l1_node->mask |= bitmask;
		return offset(l1_node->child, table.component_struct_stride * prefix);
	}

	void ecs_t::table_remove(ecs_component_table_t& table, entity_id_t id)
	{
		SFG_ASSERT(table.l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;
		table_calculate_indices(id, l0, l1, bit);

		ecs_node_t* l0_node = table.l0_nodes + l0;
		if (l0_node->mask == 0)
			return;

		const u64	bitmask = 1llu << bit;
		ecs_node_t* l1_node = reinterpret_cast<ecs_node_t*>(l0_node->child) + l1;

		if ((l1_node->mask & bitmask) == 0)
			return;

		if (table.component_struct_stride > 0)
		{
			const u32 prefix = popcount(l1_node->mask & (bitmask - 1ull));
			const u32 count	 = popcount(l1_node->mask);

			if (prefix + 1 < count)
			{
				void* base = l1_node->child;
				SFG_MEMMOVE(offset(base, prefix * table.component_struct_stride), offset(base, (prefix + 1) * table.component_struct_stride), (count - prefix - 1) * table.component_struct_stride);
			}
		}

		l1_node->mask &= ~bitmask;

		if (l1_node->mask == 0)
		{
			SFG_ALIGNED_FREE(l1_node->child);
			l1_node->child = nullptr;

			l0_node->mask &= ~(1llu << l1);

			if (l0_node->mask == 0)
			{
				SFG_ALIGNED_FREE(l0_node->child);
				l0_node->child = nullptr;
			}
		}
	}

	void ecs_t::inner_join(span_t<const ecs_component_table_ref_t> table_refs, inner_join_fn fn)
	{
		if (table_refs.size == 0 || table_refs.size > ECS_INNER_JOIN_MAX_TABLES || fn == nullptr)
			return;

		u32		  required_count						 = 0;
		bool	  is_required[ECS_INNER_JOIN_MAX_TABLES] = {};
		const u32 table_count							 = static_cast<u32>(table_refs.size);

		for (u32 i = 0; i < table_count; ++i)
		{
			if (table_refs.data[i].table == nullptr)
				return;

			const bool optional = (table_refs.data[i].flags & ecs_component_table_flags_optional) != 0;
			const bool excluded = (table_refs.data[i].flags & ecs_component_table_flags_excluded) != 0;

			is_required[i] = !optional && !excluded;
			if (is_required[i])
				required_count++;
		}

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
			table_calculate_indices(entity_index, l0, l1, bit);

			const ecs_node_t* l1_nodes[ECS_INNER_JOIN_MAX_TABLES] = {};
			u64				  include_mask						  = ~0ull;
			u64				  exclude_mask						  = 0ull;

			for (u32 i = 0; i < table_count; i++)
			{
				const ecs_node_t* l0_node = table_refs.data[i].table->l0_nodes + l0;
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
					if (table_refs.data[i].table->component_struct_stride == 0 || (l1_node->mask & bitmask) == 0)
					{
						row_ptrs[i] = nullptr;
						continue;
					}

					const u32 prefix = popcount(l1_node->mask & (bitmask - 1ull));
					row_ptrs[i]		 = offset(l1_node->child, prefix * table_refs.data[i].table->component_struct_stride);
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

	void ecs_t::table_calculate_indices(entity_id_t id, u32& l0_out, u32& l1_out, u32& bit_out)
	{
		const u32 within = id % ECS_L0_SPAN;
		l0_out			 = id / ECS_L0_SPAN;
		l1_out			 = within / ECS_L1_SPAN;
		bit_out			 = within % ECS_L1_SPAN;
	}

	bool ecs_t::advance_table_entity_index(const ecs_component_table_t& table, entity_id_t& index)
	{
		index = align_down_to_chunk(index);

		while (index < ECS_MAX_ENTITIES)
		{
			u32 l0	= 0;
			u32 l1	= 0;
			u32 bit = 0;
			table_calculate_indices(index, l0, l1, bit);

			const ecs_node_t* l0_node = table.l0_nodes + l0;
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

	size_t ecs_t::align_up(size_t value, size_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
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
