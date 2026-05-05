// Copyright (c) 2025 Inan Evin
#pragma once

#include "atlas_manager.hpp"
#include "common_resources.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <cstddef>

namespace sfg
{
	class istream_t;

	class resource_manager_t
	{
	public:
		static resource_manager_t& get();

		resource_manager_t();
		~resource_manager_t();

		resource_manager_t(const resource_manager_t&)			 = delete;
		resource_manager_t& operator=(const resource_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(size_t resource_memory_size);
		void uninit();
		void drain();
		void wait_for_all();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		resource_state_e		load_resource(sid_t hash, span_t<u8> data, resource_type_e type);
		void					unload_resource(sid_t hash);
		const resource_entry_t* find_entry(u64 hash) const;

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
			u64				 hash		  = 0;
			chunk_handle32_t runtime	  = {};
			chunk_handle32_t internals	  = {};
			span_t<u8>		 runtime_data = {};
			span_t<u8>		 payload	  = {};
			resource_type_e	 type		  = resource_type_e::invalid;
			bool			 success	  = false;
		};

		struct queues_t;

		resource_entry_t* find_entry(u64 hash);
		void			  fire_loads();
		void			  drain_completed();
		void			  drain_render_completed();
		void			  drain_unloads();
		void			  free_entry_chunks(resource_entry_t& entry);
		void			  free_entry_full_data(resource_entry_t& entry);

	private:
		chunk_allocator_t					_memory;
		hash_map_t<sid_t, resource_entry_t> _entries;
		atlas_manager_t						_atlas_manager;
		vector_t<load_request_t>			_loads;
		vector_t<u64>						_unloads;
		queues_t*							_queues	 = nullptr;
		atomic_t<u32>						_pending = 0;
	};
}
