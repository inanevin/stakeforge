// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include "io/assert.hpp"

namespace sfg
{
	void resource_manager_t::init(size_t resource_memory_size)
	{
		SFG_ASSERT(resource_memory_size != 0);
		_memory.init(resource_memory_size);
	}

	void resource_manager_t::uninit()
	{
		_entries.clear();
		_type_descs.clear();
		_memory.uninit();
	}

	resource_entry_t* resource_manager_t::find_entry(u64 hash)
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	const resource_entry_t* resource_manager_t::find_entry(u64 hash) const
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	void resource_manager_t::register_type_desc(const resource_type_desc_t& desc)
	{
		SFG_ASSERT(desc.type != 0);
		SFG_ASSERT(_type_descs.find(desc.type) == _type_descs.end());
		_type_descs.emplace(desc.type, desc);
	}

	resource_type_desc_t* resource_manager_t::find_type_desc(u32 type)
	{
		auto it = _type_descs.find(type);
		if (it == _type_descs.end())
			return nullptr;

		return &it->second;
	}

	const resource_type_desc_t* resource_manager_t::find_type_desc(u32 type) const
	{
		auto it = _type_descs.find(type);
		if (it == _type_descs.end())
			return nullptr;

		return &it->second;
	}
}
