// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_storage.hpp"
#include "common_resources.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>
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
		void init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config = {});
		void uninit();
		void flush();
		void wait_for_all();
		void wait_for_all_complete();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		// Transfers uncompressed resource payload ownership to resource manager.
		resource_state_e		load_resource(sid_t hash, const char* debug_name, span_t<u8> data, resource_type_e type);
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
		animation_storage_t							_animation_storage;
		chunk_allocator_t							_memory;
		hash_map_t<sid_t, resource_entry_t>			_entries;
		ui::glyph_atlas_t							_glyph_atlas;
		vector_t<load_request_t>					_loads;
		vector_t<u64>								_unloads;
		atomic_t<u32>								_pending = 0;
	};
}
