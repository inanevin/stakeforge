// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_storage.hpp"
#include "common_resources.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

namespace sfg
{
	class istream_t;
	class resource_file_system_t;

	class resource_manager_t final
	{
	public:
		static resource_manager_t& get();

		resource_manager_t()									 = default;
		~resource_manager_t()									 = default;
		resource_manager_t(const resource_manager_t&)			 = delete;
		resource_manager_t& operator=(const resource_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(resource_file_system_t& resource_file_system, size_t resource_memory_size);
		void init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config = {});
		void uninit_atlases();
		void uninit();
		void flush();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		resource_state_e		load_resource(sid_t hash, resource_type_e type, bool bypass_async = false);
		void					unload_resource(sid_t hash);
		const resource_entry_t* find_entry(u64 hash) const;
		void					drain_atlases(u8 frame_slot);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		template <typename T> inline const T* find_internals(u64 hash)
		{
			const resource_entry_t* entry = find_entry(hash);
			if (entry == nullptr || entry->state != resource_state_e::ready || entry->internals.size == 0)
				return nullptr;
			return _memory.get<T>(entry->internals);
		}

		template <typename T> inline const T* find_runtime(u64 hash)
		{
			const resource_entry_t* entry = find_entry(hash);
			if (entry == nullptr || entry->state == resource_state_e::failed || entry->runtime.size == 0)
				return nullptr;
			return _memory.get<T>(entry->runtime);
		}

		inline bool is_ready(u64 hash) const
		{
			const resource_entry_t* entry = find_entry(hash);
			return entry != nullptr && entry->state == resource_state_e::ready;
		}

		inline resource_state_e get_entry_state(u64 hash) const
		{
			const resource_entry_t* entry = find_entry(hash);
			return entry != nullptr ? entry->state : resource_state_e::failed;
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline chunk_allocator_t& get_memory()
		{
			return _memory;
		}

		inline const chunk_allocator_t& get_memory() const
		{
			return _memory;
		}

		inline animation_storage_t& get_animation_storage()
		{
			return _animation_storage;
		}

		inline const animation_storage_t& get_animation_storage() const
		{
			return _animation_storage;
		}

		inline ui::glyph_atlas_t& get_glyph_atlas()
		{
			return _glyph_atlas;
		}

		inline const ui::glyph_atlas_t& get_glyph_atlas() const
		{
			return _glyph_atlas;
		}

		inline u32 get_pending_count() const
		{
			return _pending.load(std::memory_order_acquire);
		}

		inline resource_file_system_t& get_resource_file_system()
		{
			SFG_ASSERT(_resource_file_system != nullptr);
			return *_resource_file_system;
		}

	private:
		struct load_request_t
		{
			u64	 hash	 = 0;
			bool success = false;
		};

		resource_entry_t* find_entry(u64 hash);
		void			  enqueue_async_load(resource_entry_t entry, const resource_type_desc_t& desc);
		load_request_t	  run_async_load(resource_entry_t entry, const resource_type_desc_t& desc);
		void			  flush_completed_loads();
		void			  flush_unloads();
		void			  unload_entry(resource_entry_t& entry);
		void			  free_entry(resource_entry_t& entry);

	private:
		moodycamel::ConcurrentQueue<load_request_t> _completed;
		animation_storage_t							_animation_storage;
		chunk_allocator_t							_memory;
		hash_map_t<sid_t, resource_entry_t>			_entries;
		ui::glyph_atlas_t							_glyph_atlas;
		vector_t<u64>								_unloads;
		resource_file_system_t*						_resource_file_system = nullptr;
		atomic_t<u32>								_pending			  = 0;
	};
}
