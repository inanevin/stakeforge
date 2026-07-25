// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

#include <sfg/platform/time.hpp>

namespace sfg
{
	resource_manager_t& resource_manager_t::get()
	{
		static resource_manager_t instance;
		return instance;
	}

	void resource_manager_t::init(resource_file_system_t& resource_file_system, size_t resource_memory_size)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(resource_memory_size != 0);
		SFG_ASSERT(_reload_listeners.empty());
		_resource_file_system = &resource_file_system;
		_generation			  = 0;
		_memory.init(resource_memory_size);
		_animation_storage.init();
		_entries.reserve(256);
	}

	void resource_manager_t::init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		_glyph_atlas.init(glyph_atlas_config);
	}

	void resource_manager_t::uninit_atlases()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		if (_glyph_atlas.is_initialized())
			_glyph_atlas.uninit();
	}

	void resource_manager_t::uninit()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(_reload_listeners.empty());

		_texture_streamer.flush_completed(*this);

		for (auto& pair : _entries)
		{
			unload_entry(pair.second);
			_generation++;
		}
		_entries.clear();

		uninit_atlases();
		_animation_storage.uninit();
		_memory.uninit();
		_reload_listeners.clear();
		_resource_file_system = nullptr;
		_generation			  = 0;
	}

	void resource_manager_t::flush()
	{
		SFG_ASSERT(is_main_thread());

		_texture_streamer.flush_completed(*this);
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, resource_type_e type)
	{
		SFG_ASSERT(is_main_thread());

		auto	   it	  = _entries.find(hash);
		const bool exists = it != _entries.end();

		if (exists)
		{
			it->second.ref_count++;
			_generation++;
			return it->second.state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);

		if (desc == nullptr)
		{
			SFG_WARN("failed loading resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (desc->load == nullptr)
		{
			SFG_WARN("failed loading resource, load not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		ostream_t header_stream = {};

		if (!_resource_file_system->read_resource(hash, 0, sizeof(resource_header_t), header_stream))
		{
			SFG_WARN("failed reading resource header: {0}", hash);
			return resource_state_e::failed;
		}

		istream_t		  header_data = {};
		resource_header_t header	  = {};

		header_data.open(header_stream.get_raw(), header_stream.get_size());
		header.deserialize(header_data);
		SFG_ASSERT(header.type == type);

		const char*		   debug_name	  = header.debug_name;
		const size_t	   payload_offset = sizeof(resource_header_t) + static_cast<size_t>(header.dependency_count) * sizeof(resource_dependency_t);
		resource_context_t ctx{*this};

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.source_ticks	   = header.source_tick;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::failed;
		entry.debug_name	   = _memory.allocate_text(debug_name);

		if (header.dependency_count != 0)
		{
			entry.dependencies	   = _memory.allocate<resource_dependency_t>(header.dependency_count);
			entry.dependency_count = header.dependency_count;

			ostream_t dependency_stream = {};

			if (!_resource_file_system->read_resource(hash, sizeof(resource_header_t), payload_offset - sizeof(resource_header_t), dependency_stream))
			{
				SFG_WARN("failed reading resource dependencies for {0}", header.debug_name);
				free_entry(entry);
				return resource_state_e::failed;
			}

			istream_t dependency_data = {};

			dependency_data.open(dependency_stream.get_raw(), dependency_stream.get_size());
			resource_dependency_t* deps = _memory.get<resource_dependency_t>(entry.dependencies);

			for (u32 i = 0; i < header.dependency_count; i++)
			{
				dependency_data >> deps[i];

				if (load_resource(deps[i].handle, deps[i].type) == resource_state_e::failed)
					SFG_WARN("failed loading dependency for {0}", header.debug_name);
			}
		}

		auto [entry_it, inserted] = _entries.emplace(hash, entry);

		SFG_ASSERT(inserted);
		resource_entry_t& loaded_entry = entry_it->second;

		if (!desc->load(loaded_entry, ctx, *_resource_file_system, payload_offset))
		{
			SFG_WARN("failed loading resource: {0} {1}", debug_name, hash);
			unload_dependencies(loaded_entry);
			free_entry(loaded_entry);
			_entries.erase(entry_it);
			return resource_state_e::failed;
		}

		loaded_entry.state = resource_state_e::ready;
		_generation++;
		SFG_TRACE("loaded resource: {0}", debug_name);

		return _entries.find(hash)->second.state;
	}

	resource_state_e resource_manager_t::reload_resource(sid_t hash)
	{
		SFG_ASSERT(is_main_thread());

		auto it = _entries.find(hash);

		if (it == _entries.end())
		{
			SFG_WARN("can't reload resource as it's not loaded! {0}", hash);
			return resource_state_e::failed;
		}

		resource_entry_t&	  entry		= it->second;
		const u32			  ref_count = entry.ref_count;
		const resource_type_e type		= entry.type;

		SFG_ASSERT(ref_count != 0);

		unload_dependencies(entry);
		unload_entry(entry);
		_entries.erase(it);
		_generation++;

		const resource_state_e state = load_resource(hash, type);

		if (state == resource_state_e::failed)
			return state;

		auto reloaded_it = _entries.find(hash);
		SFG_ASSERT(reloaded_it != _entries.end());
		reloaded_it->second.ref_count = ref_count;

		notify_reload(hash, type);

		return state;
	}

	resource_reload_listener_handle_t resource_manager_t::add_reload_listener(resource_reload_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(fn != nullptr);

		const resource_reload_listener_handle_t handle	 = _reload_listeners.emplace();
		resource_reload_listener_t&				listener = _reload_listeners.get(handle);
		listener.fn										 = fn;
		listener.user_data								 = user_data;

		return handle;
	}

	void resource_manager_t::remove_reload_listener(resource_reload_listener_handle_t handle)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(_reload_listeners.is_valid(handle));

		_reload_listeners.remove(handle);
	}

	void resource_manager_t::notify_reload(sid_t resource_id, resource_type_e resource_type)
	{
		for (auto it = _reload_listeners.begin_handle(); it != _reload_listeners.end_handle(); ++it)
		{
			const resource_reload_listener_handle_t handle	 = *it;
			const resource_reload_listener_t&		listener = _reload_listeners.get(handle);

			listener.fn(*this, resource_id, resource_type, listener.user_data);
		}
	}

	resource_state_e resource_manager_t::load_resource_runtime(sid_t hash, resource_type_e type, istream_t& stream)
	{
		SFG_ASSERT(is_main_thread());

		auto	   it	  = _entries.find(hash);
		const bool exists = it != _entries.end();
		if (exists)
		{
			it->second.ref_count++;
			_generation++;
			return it->second.state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);
		if (desc == nullptr)
		{
			SFG_WARN("failed loading runtime resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (desc->runtime_load == nullptr)
		{
			SFG_WARN("failed loading runtime resource, runtime load not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::failed;
		entry.debug_name	   = _memory.allocate_text("runtime_resource");

		auto [entry_it, inserted] = _entries.emplace(hash, entry);
		SFG_ASSERT(inserted);
		resource_entry_t&  loaded_entry = entry_it->second;
		resource_context_t ctx{*this};

		if (!desc->runtime_load(loaded_entry, ctx, stream))
		{
			SFG_WARN("failed loading runtime resource: {0}", hash);
			free_entry(loaded_entry);
			_entries.erase(entry_it);
			return resource_state_e::failed;
		}

		loaded_entry.state = resource_state_e::ready;
		_generation++;
		SFG_TRACE("loaded runtime resource: {0}", hash);
		return _entries.find(hash)->second.state;
	}

	void resource_manager_t::unload_resource(sid_t hash, bool force)
	{
		SFG_ASSERT(is_main_thread());

		auto it = _entries.find(hash);

		if (it == _entries.end())
		{
			SFG_WARN("can't unload resource as it's not loaded! {0}", hash);
			return;
		}

		resource_entry_t& entry = it->second;

		if (entry.ref_count == 0)
			return;

		if (force)
			entry.ref_count = 0;
		else
			entry.ref_count--;

		_generation++;

		if (entry.ref_count != 0)
			return;

		unload_dependencies(entry);

		unload_entry(entry);
		_entries.erase(it);
		_generation++;
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
		SFG_ASSERT(is_main_thread());

		_glyph_atlas.drain_uploads(frame_slot);
	}

	void resource_manager_t::unload_dependencies(resource_entry_t& entry)
	{
		if (entry.dependency_count == 0)
			return;

		resource_dependency_t* dependencies = _memory.get<resource_dependency_t>(entry.dependencies);

		for (u32 i = 0; i < entry.dependency_count; i++)
		{
			if (dependencies[i].handle != NULL_SID)
				unload_resource(dependencies[i].handle);
		}
	}

	void resource_manager_t::unload_entry(resource_entry_t& entry)
	{
		const resource_type_desc_t* desc = find_resource_type_desc(entry.type);
		SFG_ASSERT(desc != nullptr);

		const char* dbg = reinterpret_cast<const char*>(_memory.get(entry.debug_name.head));
		SFG_TRACE("unloaded resource: {0}", dbg);

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

		if (entry.dependencies)
		{
			_memory.free(entry.dependencies);
			entry.dependencies = {};
		}
	}

}
