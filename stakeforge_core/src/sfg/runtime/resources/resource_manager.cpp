// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/job/job_system.hpp>

#include <thread>

namespace sfg
{
	resource_manager_t::resource_manager_t()  = default;
	resource_manager_t::~resource_manager_t() = default;

	void resource_manager_t::init(u32 max_resources, size_t resource_memory_size)
	{
		SFG_ASSERT(resource_memory_size != 0);
		_memory.init(resource_memory_size);
		_entries.reserve(256);
		_loads.reserve(256);
		_unloads.reserve(256);
	}

	void resource_manager_t::uninit()
	{
		wait_for_all();
		_entries.clear();
		_memory.uninit();
	}

	void resource_manager_t::tick()
	{
		// update completed ones, fire awaiting, clean unloaded.

		drain_completed();

		if (!_loads.empty())
			fire_loads();

		if (!_unloads.empty() && _pending.load(std::memory_order_acquire) == 0)
		{
			for (u64 hash : _unloads)
			{
				auto it = _entries.find(hash);
				SFG_ASSERT(it != _entries.end());

				resource_entry_t& e = it->second;
				if (e.cpu_data)
					_memory.free(e.cpu_data);
				if (e.internals)
					_memory.free(e.internals);

				_entries.erase(it);
			}
		}
	}

	void resource_manager_t::wait_for_all()
	{
		if (!_loads.empty())
			fire_loads();

		tf::Executor& ex = job_system_t::get().get_executor();
		if (ex.this_worker_id() >= 0)
		{
			ex.corun_until([this] { return _pending.load(std::memory_order_acquire) == 0; });
		}
		else
		{
			while (_pending.load(std::memory_order_acquire) != 0)
				std::this_thread::yield();
		}

		drain_completed();
	}

	void resource_manager_t::fire_loads()
	{
		const u32 n = static_cast<u32>(_loads.size());
		_pending.fetch_add(n, std::memory_order_relaxed);

		for (const load_request_t& req : _loads)
		{
			job_system_t::get().silent_async([this, req] {
				resource_entry_t*			entry  = find_entry(req.hash);
				const resource_type_desc_t* desc   = find_resource_type_desc(entry->type);
				u8*							data   = _memory.get(req.data.head);
				istream_t					stream = {};
				stream.open(data, req.data.size);

				resource_context_t ctx{*this};
				desc->load(*entry, stream, ctx);

				_completed.enqueue(req);
				_pending.fetch_sub(1, std::memory_order_release);
			});
		}

		_loads.clear();
	}

	void resource_manager_t::drain_completed()
	{
		load_request_t req = {};
		while (_completed.try_dequeue(req))
		{
			resource_entry_t* entry = find_entry(req.hash);
			SFG_ASSERT(entry);

			if (entry->state == resource_state_e::queued)
				entry->state = resource_state_e::cpu_ready;

			_memory.free(req.data);
		}
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, span_t<u8> data, resource_type_e type)
	{
		resource_entry_t* existing = find_entry(hash);
		if (existing)
		{
			existing->ref_count++;
			return existing->state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);
		if (desc == nullptr)
		{
			SFG_ERR("failed loading resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.cpu_data		   = _memory.allocate_bytes(desc->data_size, desc->data_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::queued;
		_entries.emplace(hash, entry);

		const chunk_handle32_t stored = _memory.allocate_bytes(data.size, 8);
		u8*					   raw	  = _memory.get(stored.head);
		SFG_MEMCPY(raw, data.data, data.size);

		_loads.push_back({
			.hash = hash,
			.data = stored,
		});

		return entry.state;
	}

	void resource_manager_t::unload_resource(sid_t hash)
	{
		resource_entry_t* entry = find_entry(hash);
		if (!entry)
		{
			SFG_ERR("failed unloading resource, not found! {0}", hash);
			return;
		}

		if (entry->ref_count == 0)
			return;

		entry->ref_count--;

		if (entry->ref_count == 0)
			_unloads.push_back(hash);
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
}
