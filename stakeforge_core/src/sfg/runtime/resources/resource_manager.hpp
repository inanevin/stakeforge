// Copyright (c) 2025 Inan Evin
#pragma once

#include "atlas_manager.hpp"
#include "common_resources.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

namespace sfg
{
	class istream_t;

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

		void init(size_t resource_memory_size);
		void uninit();
		void flush();
		void wait_for_all();
		void wait_for_all_complete();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		// transfers data ownership to resource manager.
		resource_state_e		load_resource(sid_t hash, const char* debug_name, span_t<u8> data, resource_type_e type);
		void					unload_resource(sid_t hash);
		const resource_entry_t* find_entry(u64 hash) const;

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
			if (entry == nullptr || entry->state < resource_state_e::cpu_ready || entry->state == resource_state_e::failed || entry->runtime.size == 0)
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

		inline atlas_manager_t& get_atlas_manager()
		{
			return _atlas_manager;
		}

		inline const atlas_manager_t& get_atlas_manager() const
		{
			return _atlas_manager;
		}

		inline u32 get_pending_count() const
		{
			return _pending.load(std::memory_order_acquire);
		}

	private:
		struct load_request_t
		{
			u64				 hash		= 0;
			resource_entry_t copy_entry = {};
			bool			 success	= false;
		};

		resource_entry_t* find_entry(u64 hash);
		void			  fire_loads(bool wait);
		void			  flush_completed_resources();
		void			  flush_completed_render_resources();
		void			  flush_unloads();
		void			  free_entry(resource_entry_t& entry);
		void			  free_entry_load_data(resource_entry_t& entry);

	private:
		moodycamel::ConcurrentQueue<load_request_t> _completed;
		chunk_allocator_t							_memory;
		hash_map_t<sid_t, resource_entry_t>			_entries;
		atlas_manager_t								_atlas_manager;
		vector_t<load_request_t>					_loads;
		vector_t<u64>								_unloads;
		atomic_t<u32>								_pending = 0;
	};
}
