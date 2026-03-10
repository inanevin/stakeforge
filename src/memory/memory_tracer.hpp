/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#ifdef SFG_ENABLE_MEMORY_TRACER
#include "data/mutex.hpp"

#include "data/hash_map.hpp"
#include "common/size_definitions.hpp"
#include "malloc_allocator_map.hpp"
#include "malloc_allocator_stl.hpp"

namespace SFG
{
#define MEMORY_STACK_TRACE_SIZE 50

	struct memory_track
	{
		void*		   ptr							  = nullptr;
		size_t		   size							  = 0;
		unsigned short stack_size					  = 0;
		void*		   stack[MEMORY_STACK_TRACE_SIZE] = {};
	};

	typedef phmap::flat_hash_map<void*, memory_track, phmap::priv::hash_default_hash<void*>, phmap::priv::hash_default_eq<void*>, malloc_allocator_map<void*>> alloc_map;
	template <typename T> using vector_malloc = std::vector<T, malloc_allocator_stl<T>>;

	struct memory_category
	{
		const char* name	   = nullptr;
		size_t		total_size = 0;
		u8			id		   = 0;
	};

	class memory_tracer
	{
	public:
		static memory_tracer& get()
		{
			static memory_tracer instance;
			return instance;
		}

		void on_allocation(void* ptr, size_t sz);
		void on_allocation(size_t sz);
		void on_free(void* ptr);
		void on_free(size_t sz);

		void push_category(const char* name);
		void pop_category();

		mutex& get_category_mtx()
		{
			return _category_mtx;
		}

		const vector_malloc<memory_category>& get_categories() const
		{
			return _categories;
		}

	protected:
		void destroy();

	private:
		memory_tracer() = default;
		~memory_tracer()
		{
			destroy();
		}

		void capture_trace(memory_track& track);
		void check_leaks();

	private:
		mutex						   _category_mtx;
		vector_malloc<memory_category> _categories;
		vector_malloc<u8>			   _category_ids;
		alloc_map					   _allocations;

		u8		  _current_active_category = 0;
		static u8 s_category_counter;
	};

#define PUSH_MEMORY_CATEGORY(NAME) SFG::memory_tracer::get().push_category(NAME)
#define POP_MEMORY_CATEGORY()	   SFG::memory_tracer::get().pop_category()
#define PUSH_ALLOCATION(PTR, SIZE) SFG::memory_tracer::get().on_allocation(PTR, SIZE)
#define PUSH_ALLOCATION_SZ(SIZE)   SFG::memory_tracer::get().on_allocation(SIZE)
#define PUSH_DEALLOCATION(PTR)	   SFG::memory_tracer::get().on_free(PTR)
#define PUSH_DEALLOCATION_SZ(SIZE) SFG::memory_tracer::get().on_free(SIZE)
}

#else

#define PUSH_MEMORY_CATEGORY(NAME)
#define POP_MEMORY_CATEGORY()
#define CHECK_LEAKS()
#define PUSH_ALLOCATION(PTR, SIZE)
#define PUSH_ALLOCATION_SZ(SIZE)
#define PUSH_DEALLOCATION(PTR)
#define PUSH_DEALLOCATION_SZ(SIZE)
#endif