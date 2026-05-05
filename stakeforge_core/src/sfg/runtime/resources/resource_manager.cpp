// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

#include <thread>

namespace sfg
{
	struct resource_manager_t::queues_t
	{
		moodycamel::ConcurrentQueue<load_request_t> completed;
	};

	resource_manager_t::resource_manager_t() : _queues(new queues_t())
	{
	}

	resource_manager_t::~resource_manager_t()
	{
		delete _queues;
	}

	resource_manager_t& resource_manager_t::get()
	{
		static resource_manager_t instance;
		return instance;
	}

	void resource_manager_t::init(size_t resource_memory_size)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(resource_memory_size != 0);
		_memory.init(resource_memory_size);
		_atlas_manager.init();
		_entries.reserve(256);
		_loads.reserve(256);
		_unloads.reserve(256);
	}

	void resource_manager_t::uninit()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		wait_for_all();

		while (true)
		{
			bool any_pending = false;
			for (auto& kv : _entries)
			{
				if (kv.second.state == resource_state_e::internals_queued)
				{
					any_pending = true;
					break;
				}
			}
			if (!any_pending)
				break;

			render_resources_t::get().drain();
			drain_render_completed();
		}

		for (auto& kv : _entries)
		{
			resource_entry_t&			e	 = kv.second;
			const resource_type_desc_t* desc = find_resource_type_desc(e.type);

			resource_context_t ctx{*this};
			if (e.state == resource_state_e::ready || e.state == resource_state_e::cpu_ready)
			{
				if (desc->destroy_internals != nullptr)
					desc->destroy_internals(e, ctx);
			}
			free_entry_chunks(e);
		}
		_entries.clear();

		render_resources_t::get().drain();

		_atlas_manager.uninit();
		_memory.uninit();
	}

	void resource_manager_t::drain()
	{
		drain_completed();
		drain_render_completed();
		if (!_loads.empty())
			fire_loads();
		drain_unloads();
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
		drain_render_completed();
		drain_unloads();
	}

	void resource_manager_t::fire_loads()
	{
		const u32 n = static_cast<u32>(_loads.size());
		_pending.fetch_add(n, std::memory_order_relaxed);

		for (load_request_t& req : _loads)
		{
			job_system_t::get().silent_async([this, req]() mutable {
				const resource_type_desc_t* desc = find_resource_type_desc(req.type);

				u8* runtime_dst = _memory.get(req.runtime.head);
				SFG_MEMCPY(runtime_dst, req.runtime_data.data, req.runtime_data.size);

				resource_entry_t local = {};
				local.hash			   = req.hash;
				local.runtime		   = req.runtime;
				local.internals		   = req.internals;
				local.payload		   = req.payload;
				local.type			   = req.type;

				resource_context_t ctx{*this};
				req.success = desc->load(local, ctx);

				_queues->completed.enqueue(req);
				_pending.fetch_sub(1, std::memory_order_release);
			});
		}

		_loads.resize(0);
	}

	void resource_manager_t::drain_completed()
	{
		load_request_t req = {};
		while (_queues->completed.try_dequeue(req))
		{
			resource_entry_t* entry = find_entry(req.hash);
			SFG_ASSERT(entry);
			SFG_ASSERT(entry->state == resource_state_e::queued);

			if (!req.success)
			{
				entry->state = resource_state_e::failed;
				free_entry_chunks(*entry);
				continue;
			}

			entry->state					 = resource_state_e::cpu_ready;
			const resource_type_desc_t* desc = find_resource_type_desc(entry->type);

			if (desc->create_internals == nullptr)
			{
				entry->state = resource_state_e::ready;
				free_entry_full_data(*entry);
				continue;
			}

			resource_context_t				ctx{*this};
			const create_internals_result_e r = desc->create_internals(*entry, ctx);
			if (r == create_internals_result_e::failed)
			{
				entry->state = resource_state_e::failed;
				free_entry_chunks(*entry);
			}
			else if (r == create_internals_result_e::queued)
			{
				SFG_ASSERT(desc->complete_internals != nullptr);
				entry->state = resource_state_e::internals_queued;
			}
			else
			{
				entry->state = resource_state_e::ready;
				free_entry_full_data(*entry);
			}
		}
	}

	void resource_manager_t::drain_render_completed()
	{
		render_resource_completion_t completion = {};
		while (render_resources_t::get().try_dequeue_completion(completion))
		{
			resource_entry_t* entry = find_entry(completion.hash);
			SFG_ASSERT(entry);
			SFG_ASSERT(entry->type == completion.type);
			SFG_ASSERT(entry->state == resource_state_e::internals_queued);

			const resource_type_desc_t* desc = find_resource_type_desc(entry->type);
			SFG_ASSERT(desc->complete_internals != nullptr);

			resource_context_t				  ctx{*this};
			const complete_internals_result_e r = desc->complete_internals(*entry, ctx, completion);
			if (r == complete_internals_result_e::ready)
			{
				entry->state = resource_state_e::ready;
				free_entry_full_data(*entry);
			}
			else if (r == complete_internals_result_e::failed)
			{
				entry->state = resource_state_e::failed;
				free_entry_chunks(*entry);
			}
		}
	}

	void resource_manager_t::drain_unloads()
	{
		if (_unloads.empty())
			return;

		frame_vector_t<u64> still_pending;
		for (u64 hash : _unloads)
		{
			auto it = _entries.find(hash);
			SFG_ASSERT(it != _entries.end());

			resource_entry_t&			e	 = it->second;
			const resource_type_desc_t* desc = find_resource_type_desc(e.type);

			if (e.state == resource_state_e::queued || e.state == resource_state_e::internals_queued)
			{
				still_pending.push_back(hash);
				continue;
			}

			resource_context_t ctx{*this};
			if (e.state == resource_state_e::ready || e.state == resource_state_e::cpu_ready)
			{
				if (desc->destroy_internals != nullptr)
					desc->destroy_internals(e, ctx);
			}

			free_entry_chunks(e);
			_entries.erase(it);
		}

		_unloads.resize(0);
		for (u64 h : still_pending)
			_unloads.push_back(h);
	}

	void resource_manager_t::free_entry_chunks(resource_entry_t& entry)
	{
		if (entry.runtime)
		{
			_memory.free(entry.runtime);
			entry.runtime = chunk_handle32_t{};
		}
		if (entry.internals)
		{
			_memory.free(entry.internals);
			entry.internals = chunk_handle32_t{};
		}
		free_entry_full_data(entry);
	}

	void resource_manager_t::free_entry_full_data(resource_entry_t& entry)
	{
		if (entry.full_data.data != nullptr)
		{
			delete[] entry.full_data.data;
			entry.full_data = {};
		}
		entry.payload = {};
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, span_t<u8> data, resource_type_e type)
	{
		auto it = _entries.find(hash);
		if (it != _entries.end())
		{
			it->second.ref_count++;
			return it->second.state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);
		if (desc == nullptr)
		{
			SFG_ERR("failed loading resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (data.size <= sizeof(resource_header_t))
		{
			SFG_ERR("data does not contain a resource header!");
			return resource_state_e::failed;
		}

		istream_t peek;
		peek.open(data.data, data.size);
		resource_header_t header = {};
		header.deserialize(peek);

		if (header.magic != desc->wire_magic || header.version != desc->wire_version)
		{
			SFG_ERR("invalid cooked resource, magic or version does not match! [magic-desc_magic]: {0} - {1}, [version-desc_version]: {2} - {3}", header.magic, desc->wire_magic, header.version, desc->wire_version);
			return resource_state_e::failed;
		}

		if (header.payload_offset < sizeof(resource_header_t) || data.size < static_cast<size_t>(header.payload_offset) + header.payload_size)
		{
			SFG_ERR("invalid cooked resource, data layout is corrupt!");
			return resource_state_e::failed;
		}

		const size_t cooked_runtime_size = header.payload_offset - sizeof(resource_header_t);
		if (cooked_runtime_size > desc->runtime_size)
		{
			SFG_ERR("invalid cooked resource, runtime portion exceeds runtime chunk size!");
			return resource_state_e::failed;
		}

		const span_t<u8> runtime_data = {data.data + sizeof(resource_header_t), cooked_runtime_size};
		const span_t<u8> payload	  = {data.data + header.payload_offset, header.payload_size};

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.full_data		   = data;
		entry.payload		   = payload;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::queued;
		_entries.emplace(hash, entry);

		_loads.push_back({
			.hash		  = hash,
			.runtime	  = entry.runtime,
			.internals	  = entry.internals,
			.runtime_data = runtime_data,
			.payload	  = payload,
			.type		  = type,
			.success	  = false,
		});

		return entry.state;
	}

	void resource_manager_t::unload_resource(sid_t hash)
	{
		resource_entry_t* entry = find_entry(hash);
		SFG_ASSERT(entry);

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
