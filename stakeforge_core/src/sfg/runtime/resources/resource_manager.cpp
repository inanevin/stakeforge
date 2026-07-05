// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

namespace sfg
{
	resource_manager_t& resource_manager_t::get()
	{
		static resource_manager_t instance;
		return instance;
	}

	void resource_manager_t::init(resource_file_system_t& resource_file_system, size_t resource_memory_size)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(resource_memory_size != 0);
		_resource_file_system = &resource_file_system;
		_memory.init(resource_memory_size);
		_animation_storage.init();
		_entries.reserve(256);
		_unloads.reserve(256);
	}

	void resource_manager_t::init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		_glyph_atlas.init(glyph_atlas_config);
	}

	void resource_manager_t::uninit_atlases()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		if (_glyph_atlas.is_initialized())
			_glyph_atlas.uninit();
	}

	void resource_manager_t::uninit()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		flush_completed_loads();
		SFG_ASSERT(_pending.load(std::memory_order_acquire) == 0);

		for (auto& pair : _entries)
			unload_entry(pair.second);
		_entries.clear();
		_unloads.resize(0);

		uninit_atlases();
		_animation_storage.uninit();
		_memory.uninit();
		_resource_file_system = nullptr;
	}

	void resource_manager_t::flush()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		flush_completed_loads();
		flush_unloads();
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, resource_type_e type, bool bypass_async)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

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

		if (desc->load == nullptr)
		{
			SFG_ERR("failed loading resource, load not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (desc->use_async_load)
			SFG_ASSERT(desc->async_load != nullptr);

		ostream_t header_stream;
		if (!_resource_file_system->read_resource(hash, 0, sizeof(resource_header_t), header_stream))
		{
			SFG_ERR("failed reading resource header: {0}", hash);
			return resource_state_e::failed;
		}

		istream_t		  header_data;
		resource_header_t header = {};
		header_data.open(header_stream.get_raw(), header_stream.get_size());
		header.deserialize(header_data);

		for (u32 i = 0; i < header.dependency_count; i++)
			load_resource(header.dependencies[i].handle, header.dependencies[i].type, bypass_async);

		const char* debug_name = header.debug_name;

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::failed;
		entry.debug_name	   = _memory.allocate_text(debug_name);

		resource_context_t ctx{*this};
		if (!desc->load(entry, ctx, *_resource_file_system))
		{
			SFG_ERR("failed loading resource: {0} {1}", debug_name, hash);
			free_entry(entry);
			return resource_state_e::failed;
		}

		entry.state = desc->use_async_load ? resource_state_e::ready_preview : resource_state_e::ready;
		_entries.emplace(hash, entry);
		if (desc->use_async_load)
		{
			if (bypass_async)
			{
				load_request_t request	= run_async_load(entry, *desc);
				auto		   entry_it = _entries.find(hash);
				SFG_ASSERT(entry_it != _entries.end());
				if (!request.success)
				{
					entry_it->second.state = resource_state_e::failed;
					SFG_ERR("failed loading async resource: {0}", entry.hash);
				}
				else
				{
					entry_it->second.state = resource_state_e::ready;
					SFG_TRACE("loaded async resource: {0}", debug_name);
				}
			}
			else
			{
				enqueue_async_load(entry, *desc);
			}
		}
		else
			SFG_TRACE("loaded resource: {0}", debug_name);
		return _entries.find(hash)->second.state;
	}

	void resource_manager_t::unload_resource(sid_t hash)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		auto it = _entries.find(hash);
		SFG_ASSERT(it != _entries.end());

		resource_entry_t& entry = it->second;
		if (entry.ref_count == 0)
			return;

		entry.ref_count--;
		if (entry.ref_count != 0)
			return;

		if (entry.state == resource_state_e::ready_preview)
		{
			_unloads.push_back(hash);
			return;
		}

		const char* dbg = reinterpret_cast<const char*>(_memory.get(entry.debug_name.head));
		SFG_TRACE("unloaded resource: {0}", dbg);

		unload_entry(entry);
		_entries.erase(it);
	}

	const resource_entry_t* resource_manager_t::find_entry(u64 hash) const
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	void resource_manager_t::drain_atlases(u8 frame_slot)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_glyph_atlas.drain_uploads(frame_slot);
	}

	void resource_manager_t::enqueue_async_load(resource_entry_t entry, const resource_type_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(job_system_t::get().is_initialized());
		SFG_ASSERT(desc.use_async_load);
		SFG_ASSERT(desc.async_load != nullptr);

		_pending.fetch_add(1, std::memory_order_release);
		const resource_type_desc_t* desc_ptr = &desc;
		job_system_t::get().silent_async([this, entry, desc_ptr]() mutable { _completed.enqueue(run_async_load(entry, *desc_ptr)); });
	}

	resource_manager_t::load_request_t resource_manager_t::run_async_load(resource_entry_t entry, const resource_type_desc_t& desc)
	{
		SFG_ASSERT(desc.use_async_load);
		SFG_ASSERT(desc.async_load != nullptr);
		SFG_ASSERT(_resource_file_system != nullptr);

		load_request_t request = {};
		request.hash		   = entry.hash;

		resource_context_t ctx{*this};
		request.success = desc.async_load(entry, ctx, *_resource_file_system);
		return request;
	}

	void resource_manager_t::flush_completed_loads()
	{
		load_request_t request = {};
		while (_completed.try_dequeue(request))
		{
			_pending.fetch_sub(1, std::memory_order_release);

			auto it = _entries.find(request.hash);
			if (it == _entries.end())
				continue;

			resource_entry_t&			entry = it->second;
			const resource_type_desc_t* desc  = find_resource_type_desc(entry.type);
			SFG_ASSERT(desc != nullptr);

			if (!request.success)
			{
				entry.state = resource_state_e::failed;
				SFG_ERR("failed loading async resource: {0}", entry.hash);
				continue;
			}

			entry.state = resource_state_e::ready;
			SFG_TRACE("loaded async resource: {0}", _memory.get_text(entry.debug_name));
		}
	}

	void resource_manager_t::flush_unloads()
	{
		auto it = _unloads.begin();
		while (it != _unloads.end())
		{
			auto entry_it = _entries.find(*it);
			if (entry_it == _entries.end())
			{
				it = _unloads.erase(it);
				continue;
			}

			resource_entry_t& entry = entry_it->second;
			if (entry.state == resource_state_e::ready_preview)
			{
				++it;
				continue;
			}

			unload_entry(entry);
			_entries.erase(entry_it);
			it = _unloads.erase(it);
		}
	}

	void resource_manager_t::unload_entry(resource_entry_t& entry)
	{
		const resource_type_desc_t* desc = find_resource_type_desc(entry.type);
		SFG_ASSERT(desc != nullptr);

		if (desc->unload != nullptr)
		{
			resource_context_t ctx{*this};
			desc->unload(entry, ctx);
		}

		free_entry(entry);
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
	}

}
