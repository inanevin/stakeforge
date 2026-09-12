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

#include "ecs_table.hpp"

#include <sfg/memory/memory.hpp>

namespace sfg
{
	ecs_component_table_t::ecs_component_table_t(ecs_component_table_t&& other) noexcept
	{
		*this = std::move(other);
	}

	ecs_component_table_t& ecs_component_table_t::operator=(ecs_component_table_t&& other) noexcept
	{
		SFG_ASSERT(_l0_nodes == nullptr);

		_type_desc					= std::exchange(other._type_desc, {});
		_l0_nodes					= std::exchange(other._l0_nodes, nullptr);
		_component_struct_stride	= std::exchange(other._component_struct_stride, 0);
		_component_struct_alignment = std::exchange(other._component_struct_alignment, 0);
		_component_type_id			= std::exchange(other._component_type_id, 0);

		return *this;
	}

	void ecs_component_table_t::set_type_desc(const ecs_component_type_desc_t& type_desc)
	{
		SFG_ASSERT(type_desc.type_id == _component_type_id);
		SFG_ASSERT(type_desc.size == _type_desc.size && type_desc.alignment == _type_desc.alignment);

		_type_desc = type_desc;
	}

	ecs_component_type_desc_t ecs_component_table_t::make_component_desc(sid_t type_id, size_t size, size_t alignment, bitmask_t<u32> flags, const char* debug_name)
	{
		ecs_component_type_desc_t desc{
			.type_id   = type_id,
			.size	   = static_cast<u32>(size),
			.alignment = static_cast<u32>(alignment),
			.flags	   = flags,
		};

		const size_t debug_name_len = std::strlen(debug_name);
		const size_t debug_name_n	= debug_name_len < sizeof(desc.debug_name) - 1 ? debug_name_len : sizeof(desc.debug_name) - 1;

		SFG_MEMCPY(desc.debug_name, debug_name, debug_name_n);
		desc.debug_name[debug_name_n] = '\0';

		return desc;
	}

	void ecs_component_table_t::init(const ecs_component_type_desc_t& type_desc)
	{
		SFG_ASSERT(_l0_nodes == nullptr);
		SFG_ASSERT(type_desc.type_id != 0);
		SFG_ASSERT(type_desc.alignment != 0);

		_l0_nodes = reinterpret_cast<ecs_node_t*>(SFG_ALIGNED_MALLOC(alignof(ecs_node_t), sizeof(ecs_node_t) * ECS_L0_SIZE));
		SFG_MEMSET(_l0_nodes, 0, sizeof(ecs_node_t) * ECS_L0_SIZE);
		_type_desc					= type_desc;
		_component_type_id			= type_desc.type_id;
		_component_struct_alignment = type_desc.alignment;
		_component_struct_stride	= (static_cast<size_t>(type_desc.size) + type_desc.alignment - 1) & ~(static_cast<size_t>(type_desc.alignment) - 1);
	}

	void ecs_component_table_t::uninit()
	{
		SFG_ASSERT(_l0_nodes != nullptr);

		for (u32 i = 0; i < ECS_L0_SIZE; i++)
		{
			ecs_node_t* node = _l0_nodes + i;

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

		SFG_ALIGNED_FREE(_l0_nodes);

		_type_desc					= {};
		_l0_nodes					= nullptr;
		_component_struct_stride	= 0;
		_component_struct_alignment = 0;
		_component_type_id			= 0;
	}

	void ecs_component_table_t::clear()
	{
		const ecs_component_type_desc_t type_desc = _type_desc;

		uninit();
		init(type_desc);
	}

	bool ecs_component_table_t::is_empty() const
	{
		SFG_ASSERT(_l0_nodes != nullptr);

		for (u32 i = 0; i < ECS_L0_SIZE; ++i)
		{
			const ecs_node_t* l0_node = _l0_nodes + i;

			if (l0_node->mask != 0)
				return false;
		}

		return true;
	}

	bool ecs_component_table_t::has(entity_id_t id) const
	{
		SFG_ASSERT(_l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;

		calculate_indices(id, l0, l1, bit);

		const ecs_node_t* l0_node = _l0_nodes + l0;

		if ((l0_node->mask & (1llu << l1)) == 0)
			return false;

		const ecs_node_t* l1_node = reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;

		return (l1_node->mask & (1llu << bit)) != 0;
	}

	void* ecs_component_table_t::get(entity_id_t id) const
	{
		SFG_ASSERT(_l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		if (_component_struct_stride == 0)
			return nullptr;

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;

		calculate_indices(id, l0, l1, bit);

		const ecs_node_t* l0_node = _l0_nodes + l0;

		if ((l0_node->mask & (1llu << l1)) == 0)
			return nullptr;

		const ecs_node_t* l1_node = reinterpret_cast<const ecs_node_t*>(l0_node->child) + l1;
		const u64		  bitmask = 1ull << bit;

		if ((l1_node->mask & bitmask) == 0)
			return nullptr;

		const u32 prefix = static_cast<u32>(std::popcount(l1_node->mask & (bitmask - 1ull)));

		return static_cast<u8*>(l1_node->child) + prefix * _component_struct_stride;
	}

	void* ecs_component_table_t::add(entity_id_t id)
	{
		SFG_ASSERT(_l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;

		calculate_indices(id, l0, l1, bit);

		ecs_node_t* l0_node = _l0_nodes + l0;

		if (l0_node->mask == 0)
		{
			const size_t alignment = std::max(alignof(ecs_node_t), static_cast<size_t>(8));
			l0_node->child		   = SFG_ALIGNED_MALLOC(alignment, sizeof(ecs_node_t) * ECS_L1_SPAN);
			SFG_MEMSET(l0_node->child, 0, sizeof(ecs_node_t) * ECS_L1_SPAN);
		}

		l0_node->mask |= 1llu << l1;

		ecs_node_t* l1_node = reinterpret_cast<ecs_node_t*>(l0_node->child) + l1;
		const u64	bitmask = 1llu << bit;

		if (_component_struct_stride == 0)
		{
			l1_node->mask |= bitmask;

			return nullptr;
		}

		if (l1_node->mask == 0)
		{
			const size_t alignment = std::max(_component_struct_alignment, static_cast<size_t>(8));
			l1_node->child		   = SFG_ALIGNED_MALLOC(alignment, _component_struct_stride * ECS_L1_SPAN);
		}

		const u32 prefix = static_cast<u32>(std::popcount(l1_node->mask & (bitmask - 1ull)));

		if ((l1_node->mask & bitmask) != 0)
			return static_cast<u8*>(l1_node->child) + _component_struct_stride * prefix;

		const u32 count = static_cast<u32>(std::popcount(l1_node->mask));

		if (count > prefix)
		{
			void* base = l1_node->child;
			SFG_MEMMOVE((static_cast<u8*>(base) + (prefix + 1) * _component_struct_stride), (static_cast<u8*>(base) + prefix * _component_struct_stride), (count - prefix) * _component_struct_stride);
		}

		l1_node->mask |= bitmask;

		return static_cast<u8*>(l1_node->child) + _component_struct_stride * prefix;
	}

	void ecs_component_table_t::remove(entity_id_t id)
	{
		SFG_ASSERT(_l0_nodes != nullptr);
		SFG_ASSERT(id < ECS_MAX_ENTITIES);

		u32 l0	= 0;
		u32 l1	= 0;
		u32 bit = 0;

		calculate_indices(id, l0, l1, bit);

		ecs_node_t* l0_node = _l0_nodes + l0;

		if (l0_node->mask == 0)
			return;

		const u64	bitmask = 1llu << bit;
		ecs_node_t* l1_node = reinterpret_cast<ecs_node_t*>(l0_node->child) + l1;

		if ((l1_node->mask & bitmask) == 0)
			return;

		if (_component_struct_stride > 0)
		{
			const u32 prefix = static_cast<u32>(std::popcount(l1_node->mask & (bitmask - 1ull)));
			const u32 count	 = static_cast<u32>(std::popcount(l1_node->mask));

			if (prefix + 1 < count)
			{
				void* base = l1_node->child;
				SFG_MEMMOVE((static_cast<u8*>(base) + prefix * _component_struct_stride), (static_cast<u8*>(base) + (prefix + 1) * _component_struct_stride), (count - prefix - 1) * _component_struct_stride);
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

}
