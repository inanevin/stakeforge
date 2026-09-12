/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#pragma once

#ifdef SFG_ENABLE_MEMORY_TRACER

#include "malloc_allocator_map.hpp"
#include "malloc_allocator_stl.hpp"

#include <sfg/data/mutex.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/common/size_definitions.hpp>

namespace sfg
{
#define MEMORY_STACK_TRACE_SIZE 50

	struct memory_track_t
	{
		void*		   ptr							  = nullptr;
		size_t		   size							  = 0;
		u8			   category_id					  = 0;
		unsigned short stack_size					  = 0;
		void*		   stack[MEMORY_STACK_TRACE_SIZE] = {};
	};

	typedef phmap::flat_hash_map<void*, memory_track_t, phmap::priv::hash_default_hash<void*>, phmap::priv::hash_default_eq<void*>, malloc_allocator_map_t<void*>> alloc_map;
	template <typename T> using vector_malloc = std::vector<T, malloc_allocator_stl_t<T>>;

	struct memory_category_t
	{
		const char* name	   = nullptr;
		size_t		total_size = 0;
		u8			id		   = 0;
	};

	class memory_tracer_t
	{
	public:
		static memory_tracer_t& get()
		{
			static memory_tracer_t instance;
			return instance;
		}

		void on_allocation(void* ptr, size_t sz);
		void on_free(void* ptr);

		void push_category(const char* name);
		void pop_category();

		mutex_t& get_category_mtx()
		{
			return _category_mtx;
		}

		const vector_malloc<memory_category_t>& get_categories() const
		{
			return _categories;
		}

	protected:
		void destroy();

	private:
		memory_tracer_t() = default;
		~memory_tracer_t()
		{
			destroy();
		}

		void					  capture_trace(memory_track_t& track);
		void					  check_leaks();
		memory_category_t*		  find_category(u8 id);
		const memory_category_t*  find_category(u8 id) const;
		static vector_malloc<u8>& category_stack();
		static u8&				  active_category_id();

	private:
		mutex_t							 _category_mtx;
		vector_malloc<memory_category_t> _categories;
		alloc_map						 _allocations;

		static u8 s_category_counter;
	};

	class memory_category_scope_t
	{
	public:
		explicit memory_category_scope_t(const char* name)
		{
			memory_tracer_t::get().push_category(name);
		}

		~memory_category_scope_t()
		{
			memory_tracer_t::get().pop_category();
		}
	};

#define SFG_MEMTRACE_CONCAT_IMPL(A, B) A##B
#define SFG_MEMTRACE_CONCAT(A, B)	   SFG_MEMTRACE_CONCAT_IMPL(A, B)
#define SFG_MEMTRACE_CATEGORY(NAME)	   sfg::memory_tracer_t::get().push_category(NAME)
#define SFG_MEMTRACE_CATEGORY_POP()	   sfg::memory_tracer_t::get().pop_category()
#define SFG_MEMTRACE_SCOPE(NAME)	   sfg::memory_category_scope_t SFG_MEMTRACE_CONCAT(_sfg_memtrace_scope_, __LINE__)(NAME)
#define SFG_MEMTRACE_ALLOC(PTR, SIZE)  sfg::memory_tracer_t::get().on_allocation(PTR, SIZE)
#define SFG_MEMTRACE_DEALLOC(PTR)	   sfg::memory_tracer_t::get().on_free(PTR)
}

#else

#define SFG_MEMTRACE_CATEGORY(NAME)
#define SFG_MEMTRACE_CATEGORY_POP()
#define SFG_MEMTRACE_SCOPE(NAME)
#define CHECK_LEAKS()
#define SFG_MEMTRACE_ALLOC(PTR, SIZE)
#define SFG_MEMTRACE_DEALLOC(PTR)
#endif
