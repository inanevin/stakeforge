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

#include "memory/pool_allocator_gen.hpp"
#include "world/world_constants.hpp"
#include "common/type_id.hpp"
#include "data/vector.hpp"
#include "memory/chunk_allocator.hpp"

namespace SFG
{
	struct sampler_desc;
	class world;
	class meta;

	enum class comp_view_result : u8
	{
		cont,
		stop
	};

	// -----------------------------------------------------------------------------
	// base impl
	// -----------------------------------------------------------------------------

	struct comp_cache_base
	{
		virtual ~comp_cache_base() = default;

		// Lifecycle
		virtual world_handle add(world_handle entity, world& w)	   = 0;
		virtual void		 remove(world_handle handle, world& w) = 0;
		virtual void		 reset(world& w)					   = 0;
		virtual bool		 is_valid(world_handle handle) const   = 0;

		// Accessors
		virtual void*		get_ptr(world_handle h)				= 0;
		virtual const void* get_const_ptr(world_handle h) const = 0;

		// Iteration
		virtual void for_each(void* ctx, comp_view_result (*fn)(void* ctx, void* elem)) noexcept					   = 0;
		virtual void for_each_handle(void* ctx, comp_view_result (*fn)(void* ctx, const world_handle&)) const noexcept = 0;
	};

	// -----------------------------------------------------------------------------
	// cache impl
	// -----------------------------------------------------------------------------

	template <typename T, int MAX_COUNT> class comp_cache final : public comp_cache_base
	{
	public:
		using cache_type = comp_cache<T, MAX_COUNT>;
		using pool_type	 = pool_allocator_gen<T, world_id, MAX_COUNT>;

		virtual ~comp_cache() = default;

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		virtual world_handle add(world_handle entity, world& w) override
		{
			if (_components.is_full())
				return {};

			const world_handle handle = _components.add();
			T&				   comp	  = _components.get(handle);
			comp._header.entity		  = entity;
			comp._header.own_handle	  = handle;
			comp.on_add(w);
			return handle;
		}

		virtual void remove(world_handle handle, world& w) override
		{
			T& comp = _components.get(handle);
			comp.on_remove(w);
			_components.remove(handle);
		}

		virtual void reset(world& w) override
		{
			for (T& comp : _components)
			{
				comp.on_remove(w);
			}

			_components.reset();
		}

		bool is_valid(world_handle handle) const override
		{
			return _components.is_valid(handle);
		}

		// -----------------------------------------------------------------------------
		// Accessors
		// -----------------------------------------------------------------------------

		void* get_ptr(world_handle h) override
		{
			return &(_components.get(h));
		}

		const void* get_const_ptr(world_handle h) const override
		{
			return &(_components.get(h));
		}

		// -----------------------------------------------------------------------------
		// Iteration
		// -----------------------------------------------------------------------------

		void for_each(void* ctx, comp_view_result (*fn)(void*, void*)) noexcept override
		{
			for (auto it = _components.begin(); it != _components.end(); ++it)
			{
				T& obj = *it;
				if (fn(ctx, &obj) == comp_view_result::stop)
					break;
			}
		}

		void for_each_handle(void* ctx, comp_view_result (*fn)(void*, const world_handle&)) const noexcept override
		{
			for (auto it = _components.handles_begin(); it != _components.handles_end(); ++it)
			{
				const world_handle& handle = *it;
				if (fn(ctx, handle) == comp_view_result::stop)
					break;
			}
		}

		inline pool_type& get_pool()
		{
			return _components;
		}

	private:
		pool_type _components;
	};

	// -----------------------------------------------------------------------------
	// comp manager
	// -----------------------------------------------------------------------------

	class component_manager
	{
	private:
		struct comp_cache_storage
		{
			comp_cache_base* cache_ptr = nullptr;
			string_id		 type	   = 0;
		};

	public:
		component_manager() = delete;
		component_manager(world& w);
		~component_manager();

		void init();
		void uninit();

		template <typename T, int MAX_COUNT> void register_cache()
		{
			comp_cache_base* cache = new comp_cache<T, MAX_COUNT>();
			const string_id	 tid   = type_id<T>::value;

			comp_cache_storage storage = {
				.cache_ptr = cache,
				.type	   = tid,
			};

			_storages.push_back(storage);
		}

		// -----------------------------------------------------------------------------
		// Lifecycle
		// -----------------------------------------------------------------------------
		world_handle add_component(string_id type, world_handle entity);
		void		 remove_component(string_id type, world_handle entity, world_handle handle);
		bool		 is_valid(string_id type, world_handle handle) const;
		void*		 get_component(string_id type, world_handle handle) const;

		template <typename T> inline world_handle add_component(world_handle entity)
		{
			const string_id type = type_id<T>::value;
			return add_component(type, entity);
		}

		template <typename T> inline void remove_component(world_handle entity, world_handle handle)
		{
			const string_id type = type_id<T>::value;
			remove_component(type, entity, handle);
		}

		template <typename T> inline bool is_valid(world_handle handle) const
		{
			const string_id type = type_id<T>::value;
			return is_valid(type, handle);
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		template <typename T> inline T& get_component(world_handle handle) const
		{
			const string_id type	 = type_id<T>::value;
			void*			ptr		 = get_component(type, handle);
			T*				type_ptr = reinterpret_cast<T*>(ptr);
			return *type_ptr;
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		// -----------------------------------------------------------------------------
		// iteration
		// -----------------------------------------------------------------------------

		template <typename T, typename Fn> inline void view(Fn&& fn)
		{
			const string_id			  type = type_id<T>::value;
			const comp_cache_storage& stg  = get_storage(type);

			auto bounce = +[](void* ctx, void* elem) -> comp_view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(*static_cast<T*>(elem));
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename T, typename Fn> inline void view_handles(Fn&& fn)
		{
			const string_id			  type = type_id<T>::value;
			const comp_cache_storage& stg  = get_storage(type);

			auto bounce = +[](void* ctx, const world_handle& handle) -> comp_view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(handle);
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each_handle(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename CACHE_TYPE, typename T> inline auto& underlying_pool()
		{
			const string_id			  type	= type_id<T>::value;
			const comp_cache_storage& stg	= get_storage(type);
			CACHE_TYPE*				  cache = static_cast<CACHE_TYPE*>(stg.cache_ptr);
			return cache->get_pool();
		}

	private:
		const comp_cache_storage& get_storage(string_id type) const;
		comp_cache_storage&		  get_storage(string_id type);

	private:
		chunk_allocator32		   _aux_memory = {};
		vector<comp_cache_storage> _storages   = {};
		world&					   _world;
	};
}
