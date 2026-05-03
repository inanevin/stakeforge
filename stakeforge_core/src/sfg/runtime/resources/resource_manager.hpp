// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/common/string_id.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>
#include <sfg/vendor/moodycamel/readerwriterqueue.h>
#include <cstddef>

namespace sfg
{
	class istream_t;

	class resource_manager_t
	{
	public:
		resource_manager_t();
		~resource_manager_t();

		resource_manager_t(const resource_manager_t&)			 = delete;
		resource_manager_t& operator=(const resource_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 max_resources, size_t resource_memory_size);
		void uninit();
		void tick();
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

		inline u32 get_pending_count() const
		{
			return _pending.load(std::memory_order_acquire);
		}

	private:
		struct load_request_t
		{
			u64				 hash = 0;
			chunk_handle32_t data = {};
		};

		resource_entry_t* find_entry(u64 hash);
		void			  fire_loads();
		void			  drain_completed();

	private:
		atomic_t<u32>								_pending = 0;
		moodycamel::ConcurrentQueue<load_request_t> _completed;
		chunk_allocator_t							_memory;
		vector_t<load_request_t>					_loads;
		vector_t<u64>								_unloads;
		hash_map_t<sid_t, resource_entry_t>			_entries;
	};
}
