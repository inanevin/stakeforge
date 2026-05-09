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

	// -----------------------------------------------------------------------------
	// PUBLIC API
	// -----------------------------------------------------------------------------

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

		wait_for_all_complete();

		for (auto& pair : _entries)
		{
			resource_entry_t&			e	 = pair.second;
			const resource_type_desc_t* desc = find_resource_type_desc(e.type);

			resource_context_t ctx{*this};
			if (e.state == resource_state_e::ready || e.state == resource_state_e::cpu_ready)
			{
				if (desc->destroy_internals != nullptr)
					desc->destroy_internals(e, ctx);
			}
			free_entry(e);
		}
		_entries.clear();
		_atlas_manager.uninit();
		_memory.uninit();
	}

	void resource_manager_t::flush()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		flush_completed_resources();
		flush_completed_render_resources();
		fire_loads(false);
		flush_unloads();
	}

	void resource_manager_t::wait_for_all()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		fire_loads(true);
		flush_completed_resources();
		flush_completed_render_resources();
		flush_unloads();
	}

	void resource_manager_t::wait_for_all_complete()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		fire_loads(true);
		flush_completed_resources();

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

			render_resources_t::get().flush();
			flush_completed_render_resources();
		}

		flush_unloads();
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, const char* debug_name, span_t<u8> data, resource_type_e type)
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

		if (data.size < sizeof(u32) * 5)
		{
			SFG_ERR("data does not contain a resource header!");
			return resource_state_e::failed;
		}

		istream_t peek;
		peek.open(data.data, data.size);
		resource_header_t header = {};
		header.deserialize(peek);

		const size_t header_disk_size = peek.tellg();

		if (header.magic != desc->wire_magic || header.version != desc->wire_version)
		{
			SFG_ERR("invalid cooked resource, magic or version does not match! [magic-desc_magic]: {0} - {1}, [version-desc_version]: {2} - {3}", header.magic, desc->wire_magic, header.version, desc->wire_version);
			return resource_state_e::failed;
		}

		if (header.payload_offset < header_disk_size || data.size < static_cast<size_t>(header.payload_offset) + header.payload_size)
		{
			SFG_ERR("invalid cooked resource, data layout is corrupt!");
			return resource_state_e::failed;
		}

		const size_t cooked_runtime_size = header.payload_offset - header_disk_size;
		if (cooked_runtime_size > desc->runtime_size)
		{
			SFG_ERR("invalid cooked resource, runtime portion exceeds runtime chunk size!");
			return resource_state_e::failed;
		}

		const span_t<u8> runtime_data = {data.data + header_disk_size, cooked_runtime_size};
		const span_t<u8> payload	  = {data.data + header.payload_offset, header.payload_size};

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.load_data		   = data;
		entry.payload		   = payload;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::queued;

		const size_t name_sz = strlen(debug_name);
		if (name_sz != 0)
			entry.debug_name = _memory.allocate_text(debug_name);

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

	const resource_entry_t* resource_manager_t::find_entry(u64 hash) const
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	// -----------------------------------------------------------------------------
	// PRIVATE API
	// -----------------------------------------------------------------------------

	void resource_manager_t::fire_loads(bool wait)
	{
		if (_loads.empty())
			return;

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

		if (wait)
		{
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
		}
	}

	void resource_manager_t::flush_completed_resources()
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
				SFG_ERR("failed loading resource: {0}", _memory.get_text(entry->debug_name));
				free_entry(*entry);
				continue;
			}

			entry->state					 = resource_state_e::cpu_ready;
			const resource_type_desc_t* desc = find_resource_type_desc(entry->type);

			if (desc->create_internals == nullptr)
			{
				entry->state = resource_state_e::ready;
				SFG_TRACE("loaded resource: {0}", _memory.get_text(entry->debug_name));
				free_entry_load_data(*entry);
				continue;
			}

			resource_context_t				ctx{*this};
			const create_internals_result_e r = desc->create_internals(*entry, ctx);
			if (r == create_internals_result_e::failed)
			{
				entry->state = resource_state_e::failed;
				SFG_ERR("failed creating internals for resource: {0}", _memory.get_text(entry->debug_name));
				free_entry(*entry);
			}
			else if (r == create_internals_result_e::queued)
			{
				SFG_ASSERT(desc->complete_internals != nullptr);
				entry->state = resource_state_e::internals_queued;
				SFG_TRACE("loaded resource and queued internals: {0}", _memory.get_text(entry->debug_name));
			}
			else
			{
				entry->state = resource_state_e::ready;
				free_entry_load_data(*entry);
				SFG_TRACE("loaded resource and internals: {0}", _memory.get_text(entry->debug_name));
			}
		}
	}

	void resource_manager_t::flush_completed_render_resources()
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
				SFG_TRACE("loaded resource internals: {0}", _memory.get_text(entry->debug_name));
				free_entry_load_data(*entry);
			}
			else if (r == complete_internals_result_e::failed)
			{
				entry->state = resource_state_e::failed;
				SFG_TRACE("failed loading resource internals: {0}", _memory.get_text(entry->debug_name));
				free_entry(*entry);
			}
		}
	}

	void resource_manager_t::flush_unloads()
	{
		if (_unloads.empty())
			return;

		auto it = _unloads.begin();
		while (it != _unloads.end())
		{
			const u64 hash = *it;

			auto ite = _entries.find(hash);
			SFG_ASSERT(ite != _entries.end());

			resource_entry_t&			e	 = ite->second;
			const resource_type_desc_t* desc = find_resource_type_desc(e.type);

			if (e.state == resource_state_e::queued || e.state == resource_state_e::internals_queued)
			{
				++it;
				continue;
			}

			resource_context_t ctx{*this};
			if (e.state == resource_state_e::ready || e.state == resource_state_e::cpu_ready)
			{
				if (desc->destroy_internals != nullptr)
					desc->destroy_internals(e, ctx);
			}

			free_entry(e);
			_entries.erase(ite);
			it = _unloads.erase(it);
		}
	}

	void resource_manager_t::free_entry(resource_entry_t& entry)
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
		if (entry.debug_name)
		{
			_memory.free(entry.debug_name);
			entry.debug_name = {};
		}
		free_entry_load_data(entry);
	}

	void resource_manager_t::free_entry_load_data(resource_entry_t& entry)
	{
		if (entry.load_data.data != nullptr)
		{
			delete[] entry.load_data.data;
			entry.load_data = {};
		}
		entry.payload = {};
	}

	resource_entry_t* resource_manager_t::find_entry(u64 hash)
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

}
