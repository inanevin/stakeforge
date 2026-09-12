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

#include "ecs_component_type.hpp"
#include "ecs_defs.hpp"

namespace sfg
{
	class ecs_t;
	struct ecs_query_range_t;
	struct ecs_query_chunk_range_t;

	struct ecs_node_t
	{
		u64	  mask;
		void* child;
	};

	static_assert(std::is_trivial_v<ecs_node_t>);
	static_assert(alignof(ecs_node_t) == 8);

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

	class ecs_component_table_t final
	{
		friend class ecs_t;
		friend struct ecs_query_range_t;
		friend struct ecs_query_chunk_range_t;

	public:
		ecs_component_table_t()												 = default;
		~ecs_component_table_t()											 = default;
		ecs_component_table_t(const ecs_component_table_t& other)			 = delete;
		ecs_component_table_t& operator=(const ecs_component_table_t& other) = delete;

		ecs_component_table_t(ecs_component_table_t&& other) noexcept;
		ecs_component_table_t& operator=(ecs_component_table_t&& other) noexcept;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const ecs_component_type_desc_t& type_desc);
		void uninit();
		void clear();

		// -----------------------------------------------------------------------------
		// table
		// -----------------------------------------------------------------------------

		static ecs_component_type_desc_t make_component_desc(sid_t type_id, size_t size, size_t alignment, bitmask_t<u32> flags, const char* debug_name);

		void* add(entity_id_t id);
		void  remove(entity_id_t id);
		void  set_type_desc(const ecs_component_type_desc_t& type_desc);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool  is_empty() const;
		bool  has(entity_id_t id) const;
		void* get(entity_id_t id) const;

		template <typename T> T& get_as(entity_id_t id) const
		{
			void* ptr = get(id);

			SFG_ASSERT(ptr != nullptr);

			return *reinterpret_cast<T*>(ptr);
		}

		template <typename T> const T& get_as_const(entity_id_t id) const
		{
			void* ptr = get(id);

			SFG_ASSERT(ptr != nullptr);

			return *reinterpret_cast<const T*>(ptr);
		}

		template <typename T> T* find_as(entity_id_t id) const
		{
			return reinterpret_cast<T*>(get(id));
		}

		template <typename T> const T* find_as_const(entity_id_t id) const
		{
			return reinterpret_cast<const T*>(get(id));
		}

		template <typename T> T& add_or_get_as(entity_id_t id)
		{
			const bool existed = has(id);
			T*		   value   = reinterpret_cast<T*>(add(id));

			if (!existed)
				*value = T{};

			return *value;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		const ecs_component_type_desc_t& get_type_desc() const
		{
			return _type_desc;
		}

		sid_t get_type_id() const
		{
			return _component_type_id;
		}

		ecs_component_table_ref_t ref() const
		{
			return {.table = this, .flags = 0};
		}

	private:
		static void calculate_indices(entity_id_t id, u32& l0_out, u32& l1_out, u32& bit_out)
		{
			const u32 within = id % ECS_L0_SPAN;

			l0_out	= id / ECS_L0_SPAN;
			l1_out	= within / ECS_L1_SPAN;
			bit_out = within % ECS_L1_SPAN;
		}

		ecs_component_type_desc_t _type_desc				  = {};
		ecs_node_t*				  _l0_nodes					  = nullptr;
		size_t					  _component_struct_stride	  = 0;
		size_t					  _component_struct_alignment = 0;
		sid_t					  _component_type_id		  = 0;
	};
}
