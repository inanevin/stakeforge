// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "data/hash_map.hpp"
#include "memory/chunk_allocator.hpp"

#include <cstddef>

namespace sfg
{
	class resource_manager_t
	{
	public:
		void init(size_t resource_memory_size);
		void uninit();

		resource_entry_t*			find_entry(u64 hash);
		const resource_entry_t*		find_entry(u64 hash) const;
		void						register_type_desc(const resource_type_desc_t& desc);
		resource_type_desc_t*		find_type_desc(resource_type_t type);
		const resource_type_desc_t* find_type_desc(resource_type_t type) const;

		inline chunk_allocator_t& get_memory()
		{
			return _memory;
		}

		inline const chunk_allocator_t& get_memory() const
		{
			return _memory;
		}

	private:
		chunk_allocator_t					 _memory;
		hash_map_t<u64, resource_entry_t>	 _entries;
		hash_map_t<u8, resource_type_desc_t> _type_descs;
	};
}
