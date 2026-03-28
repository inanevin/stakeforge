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
#include "common_resources.hpp"
#include "common/type_id.hpp"
#include "data/hash_map.hpp"
#include "data/string.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_allocator.hpp"
#include "resources/texture_raw.hpp"

#ifdef SFG_TOOLMODE
#include "io/simple_file_watcher.hpp"
#endif

namespace SFG
{
	struct sampler_desc;
	class world;
	class istream;
	class ostream;
	class meta;
	class package;

	enum class view_result : u8
	{
		cont,
		stop
	};

	// -----------------------------------------------------------------------------
	// base impl
	// -----------------------------------------------------------------------------

	struct resource_cache_base
	{
		virtual ~resource_cache_base() = default;

		// Loader I/O
		virtual void* load_from_stream(istream& stream)							= 0;
		virtual void  save_to_stream(const void* loader, ostream& stream) const = 0;

#ifdef SFG_TOOLMODE
		virtual void* load_from_file(const char* relative_file, const char* base_path)																	 = 0;
		virtual void  save_to_cache(const void* loader, const char* cache_folder_path, const char* resource_directory_path, const char* extension) const = 0;
		virtual void* load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension) const							 = 0;
		virtual void  get_dependencies(const void* loader, vector<string>& out_deps) const																 = 0;
#endif

		// Lifecycle
		virtual resource_handle add_from_loader(void* loader, world& w, string_id hash)				   = 0;
		virtual void			get_loader_sub_resources(void* loader, vector<string>& out_subs) const = 0;
		virtual void			delete_loader(void* loader) const									   = 0;
		virtual void			destroy(resource_handle handle, world& w)							   = 0;
		virtual resource_handle add(string_id hash)													   = 0;
		virtual void			remove(resource_handle handle)										   = 0;
		virtual void			reset(world& w)														   = 0;

		// Accessors
		virtual void*			get_ptr(resource_handle h)						   = 0;
		virtual const void*		get_const_ptr(resource_handle h) const			   = 0;
		virtual void*			get_by_hash_ptr(string_id hash)					   = 0;
		virtual const void*		get_by_hash_const_ptr(string_id hash) const		   = 0;
		virtual resource_handle get_handle_by_hash(string_id hash) const		   = 0;
		virtual resource_handle get_handle_by_hash_if_exists(string_id hash) const = 0;
		virtual string_id		get_hash(resource_handle handle) const			   = 0;
		virtual bool			is_valid(resource_handle handle) const			   = 0;

		// Iteration
		virtual void for_each(void* ctx, view_result (*fn)(void* ctx, void* elem)) noexcept							 = 0;
		virtual void for_each_handle(void* ctx, view_result (*fn)(void* ctx, const resource_handle&)) const noexcept = 0;

	protected:
		friend class resource_manager;

		hash_map<string_id, resource_handle> _by_hashes;
		hash_map<string_id, string>			 _paths_by_hashes;
	};

	// -----------------------------------------------------------------------------
	// cache impl
	// -----------------------------------------------------------------------------

	template <typename T, typename Loader, int MAX_COUNT> class resource_cache final : public resource_cache_base
	{
	public:
		using cache_type = resource_cache<T, Loader, MAX_COUNT>;
		using pool_type	 = pool_allocator_gen<T, resource_id, MAX_COUNT>;

		virtual ~resource_cache() = default;

		// -----------------------------------------------------------------------------
		// loader i/o
		// -----------------------------------------------------------------------------

		void* load_from_stream(istream& stream) override
		{
			Loader* loader = new Loader();
			loader->deserialize(stream);
			return loader;
		}

		void save_to_stream(const void* loader, ostream& stream) const override
		{
			static_cast<const Loader*>(loader)->serialize(stream);
		}

#ifdef SFG_TOOLMODE

		void* load_from_file(const char* relative_file, const char* base_path) override
		{
			Loader* loader = new Loader();
			if (!loader->load_from_file(relative_file, base_path))
			{
				delete loader;
				return nullptr;
			}
			return loader;
		}

		void* load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension) const override
		{
			Loader* loader = new Loader();
			if (!loader->load_from_cache(cache_folder_path, relative_path, extension))
			{
				delete loader;
				return nullptr;
			}
			return loader;
		}

		void save_to_cache(const void* loader, const char* cache_folder_path, const char* resource_directory_path, const char* extension) const override
		{
			static_cast<const Loader*>(loader)->save_to_cache(cache_folder_path, resource_directory_path, extension);
		}

		void get_dependencies(const void* loader_ptr, vector<string>& out_deps) const override
		{
			static_cast<const Loader*>(loader_ptr)->get_dependencies(out_deps);
		}

#endif

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		resource_handle add_from_loader(void* loader, world& w, string_id hash) override
		{
			resource_handle handle = add(hash);
			T&				res	   = _resources.get(handle);
			Loader*			lp	   = static_cast<Loader*>(loader);
			Loader&			lref   = *(lp);
			res.create_from_loader(lref, w, handle);
			return handle;
		}

		void get_loader_sub_resources(void* loader, vector<string>& out_subs) const override
		{
			Loader* lp = static_cast<Loader*>(loader);
			lp->get_sub_resources(out_subs);
		}

		void delete_loader(void* loader) const override
		{
			Loader* lp = static_cast<Loader*>(loader);
			delete lp;
		}

		void destroy(resource_handle handle, world& w) override
		{
			T& res = _resources.get(handle);
			res.destroy(w, handle);
		}

		resource_handle add(string_id hash) override
		{
			resource_handle handle = _resources.add();
			_by_hashes[hash]	   = handle;
			return handle;
		}

		void remove(resource_handle handle) override
		{
			_resources.remove(handle);

			// linear scan :/
			auto it = _by_hashes.begin();
			for (; it != _by_hashes.end(); ++it)
			{
				if (it->second == handle)
				{
#ifdef SFG_TOOLMODE
					auto it2 = _paths_by_hashes.find(it->first);
					if (it2 != _paths_by_hashes.end())
						_paths_by_hashes.erase(it2);
#endif

					_by_hashes.erase(it);

					break;
				}
			}
		}

		void reset(world& w) override
		{
			auto it = _resources.handles_begin();
			while (it != _resources.handles_end())
			{
				const resource_handle handle = *it;
				T&					  res	 = _resources.get(handle);
				res.destroy(w, handle);
				++it;
			}
			_by_hashes.clear();
			_resources.reset();
		}

		// -----------------------------------------------------------------------------
		// Accessors
		// -----------------------------------------------------------------------------

		void* get_ptr(resource_handle h) override
		{
			return &(_resources.get(h));
		}

		const void* get_const_ptr(resource_handle h) const override
		{
			return &(_resources.get(h));
		}

		void* get_by_hash_ptr(string_id hash) override
		{
			return &(get_by_hash(hash));
		}

		const void* get_by_hash_const_ptr(string_id hash) const override
		{
			return &(get_by_hash_const(hash));
		}

		resource_handle get_handle_by_hash(string_id hash) const override
		{
			return _by_hashes.at(hash);
		}

		resource_handle get_handle_by_hash_if_exists(string_id hash) const override
		{
			auto it = _by_hashes.find(hash);
			if (it == _by_hashes.end())
				return {};

			const resource_handle h = it->second;
			return h;
		}

		string_id get_hash(resource_handle handle) const override
		{
			for (const auto& [sid, hnd] : _by_hashes)
			{
				if (hnd == handle)
					return sid;
			}
			return 0;
		}

		bool is_valid(resource_handle handle) const override
		{
			return _resources.is_valid(handle);
		}

		// -----------------------------------------------------------------------------
		// Iteration
		// -----------------------------------------------------------------------------

		void for_each(void* ctx, view_result (*fn)(void*, void*)) noexcept override
		{
			for (auto it = _resources.begin(); it != _resources.end(); ++it)
			{
				T& obj = *it;
				if (fn(ctx, &obj) == view_result::stop)
					break;
			}
		}

		void for_each_handle(void* ctx, view_result (*fn)(void*, const resource_handle&)) const noexcept override
		{
			for (auto it = _resources.handles_begin(); it != _resources.handles_end(); ++it)
			{
				const resource_handle& handle = *it;
				if (fn(ctx, handle) == view_result::stop)
					break;
			}
		}

		inline pool_type& get_pool()
		{
			return _resources;
		}

	private:
		inline T& get_by_hash(string_id hash)
		{
			const resource_handle& h = _by_hashes.at(hash);
			return _resources.get(h);
		}

		inline const T& get_by_hash_const(string_id hash) const
		{
			const resource_handle& h = _by_hashes.at(hash);
			return _resources.get(h);
		}

	private:
		pool_type _resources;
	};

	// -----------------------------------------------------------------------------
	// Resource manager
	// -----------------------------------------------------------------------------

	class resource_manager
	{
	private:
		struct cache_storage
		{
			resource_cache_base* cache_ptr	   = nullptr;
			string_id			 type		   = 0;
			u32					 load_priority = 0;
		};

	public:
		resource_manager() = delete;
		resource_manager(world& w);
		~resource_manager();

		void init();
		void uninit();
		void tick();

#ifdef SFG_TOOLMODE
		void load_resources(const vector<string>& relative_paths, bool skip_cache = false, const char* root_directory = nullptr);
#endif

		template <typename T, typename Loader, int MAX_COUNT, int LOAD_PRIORITY> void register_cache()
		{
			using cache_type	  = resource_cache<T, Loader, MAX_COUNT>;
			cache_type*		cache = new cache_type();
			const string_id tid	  = type_id<T>::value;

			cache_storage storage = {
				.cache_ptr	   = cache,
				.type		   = tid,
				.load_priority = LOAD_PRIORITY,
			};

			if (LOAD_PRIORITY > _max_load_priority)
				_max_load_priority = LOAD_PRIORITY;

			_storages.push_back(storage);
		}

		// -----------------------------------------------------------------------------
		// Lifecycle
		// -----------------------------------------------------------------------------

		resource_handle get_or_add_sampler(const sampler_desc& desc);
		void			load_resources(const vector<string>& relative_paths, package& pkg);
		resource_handle add_resource(string_id type, string_id hash);
		void			remove_resource(string_id type, resource_handle handle);
		bool			is_valid(string_id type, resource_handle handle) const;
		void			store_relative_path(string_id type, resource_handle handle, const string& p);

		template <typename T> inline resource_handle add_resource(string_id hash)
		{
			const string_id type = type_id<T>::value;
			return add_resource(type, hash);
		}

		template <typename T> inline void remove_resource(resource_handle handle)
		{
			const string_id type = type_id<T>::value;
			remove_resource(type, handle);
		}

		template <typename T> inline bool is_valid(resource_handle handle) const
		{
			const string_id type = type_id<T>::value;
			return is_valid(type, handle);
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void*			get_resource(string_id type, resource_handle handle) const;
		void*			get_resource_by_hash(string_id type, string_id hash) const;
		resource_handle get_resource_handle_by_hash(string_id type, string_id hash) const;
		resource_handle get_resource_handle_by_hash_if_exists(string_id type, string_id hash) const;
		string_id		get_resource_hash(string_id type, resource_handle handle) const;

		template <typename T> inline T& get_resource(resource_handle handle) const
		{
			const string_id type	 = type_id<T>::value;
			void*			ptr		 = get_resource(type, handle);
			T*				type_ptr = reinterpret_cast<T*>(ptr);
			return *type_ptr;
		}

		template <typename T> inline T& get_resource_by_hash(string_id hash) const
		{
			const string_id type = type_id<T>::value;
			void*			ptr	 = get_resource_by_hash(type, hash);
			return *static_cast<T*>(ptr);
		}

		template <typename T> inline resource_handle get_resource_handle_by_hash(string_id hash) const
		{
			const string_id type = type_id<T>::value;
			return get_resource_handle_by_hash(type, hash);
		}

		template <typename T> inline resource_handle get_resource_handle_by_hash_if_exists(string_id hash) const
		{
			const string_id type = type_id<T>::value;
			return get_resource_handle_by_hash_if_exists(type, hash);
		}

		template <typename T> inline string_id get_resource_hash(resource_handle handle) const
		{
			const string_id type = type_id<T>::value;
			return get_resource_hash(type, handle);
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		inline const chunk_allocator32& get_aux() const
		{
			return _aux_memory;
		}

		inline resource_handle get_default_gui_mat() const
		{
			return _default_gui_mat;
		}

		inline resource_handle get_default_gui_text_mat() const
		{
			return _default_gui_text_mat;
		}

		inline resource_handle get_default_gui_sdf_mat() const
		{
			return _default_gui_sdf_mat;
		}

		const string& get_loaded_path_by_handle(string_id type, resource_handle h) const;

		// -----------------------------------------------------------------------------
		// iteration
		// -----------------------------------------------------------------------------
		template <typename T, typename Fn> inline void view(Fn&& fn)
		{
			const string_id		 type = type_id<T>::value;
			const cache_storage& stg  = get_storage(type);

			auto bounce = +[](void* ctx, void* elem) -> view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(*static_cast<T*>(elem));
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename T, typename Fn> inline void view_handles(Fn&& fn)
		{
			const string_id		 type = type_id<T>::value;
			const cache_storage& stg  = get_storage(type);

			auto bounce = +[](void* ctx, const resource_handle& handle) -> view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(handle);
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each_handle(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename CACHE_TYPE, typename T> inline auto& underlying_pool()
		{
			const string_id		 type  = type_id<T>::value;
			const cache_storage& stg   = get_storage(type);
			CACHE_TYPE*			 cache = static_cast<CACHE_TYPE*>(stg.cache_ptr);
			return cache->get_pool();
		}

	private:
		// -----------------------------------------------------------------------------
		// loader i/o
		// -----------------------------------------------------------------------------

		void*			load_from_stream(string_id type, istream& stream) const;
		void			save_to_stream(string_id type, const void* loader, ostream& stream) const;
		resource_handle add_from_loader(string_id type, void* loader, u32 priority, string_id hash) const;
		void			get_loader_sub_resources(string_id type, void* loader, vector<string>& out_subs) const;
		void			delete_loader(string_id type, void* loader) const;
		void			destroy(string_id type, resource_handle h);

#ifdef SFG_TOOLMODE
		void* load_from_file(string_id type, const char* relative_file, const char* base_path) const;
		void* load_from_cache(string_id type, const char* cache_folder_path, const char* relative_path, const char* extension) const;
		void  save_to_cache(string_id type, const void* loader, const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void  get_dependencies(string_id type, const void* loader, vector<string>& out_dependencies) const;

		template <typename T> inline void* load_from_file(const char* relative_file, const char* base_path) const
		{
			const string_id type = type_id<T>::value;
			return load_from_file(type, relative_file, base_path);
		}

		template <typename T> inline void* load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension) const
		{
			const string_id type = type_id<T>::value;
			return load_from_cache(type, cache_folder_path, relative_path, extension);
		}

		template <typename T> inline void save_to_cache(const void* loader, const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
		{
			const string_id type = type_id<T>::value;
			save_to_cache(type, loader, cache_folder_path, resource_directory_path, extension);
		}

		template <typename T> inline void get_dependencies(const void* loader, vector<string>& out_dependencies) const
		{
			const string_id type = type_id<T>::value;
			get_dependencies(type, loader, out_dependencies);
		}

#endif

		template <typename T> inline void* load_from_stream(istream& stream) const
		{
			const string_id type = type_id<T>::value;
			return load_from_stream(type, stream);
		}

		template <typename T> inline void save_to_stream(const void* loader, ostream& stream) const
		{
			const string_id type = type_id<T>::value;
			save_to_stream(type, loader, stream);
		}

		template <typename T> inline resource_handle add_from_loader(void* loader, u32 priority, string_id hash) const
		{
			const string_id type = type_id<T>::value;
			return add_from_loader(type, loader, priority, hash);
		}

		template <typename T> inline void destroy(resource_handle handle)
		{
			const string_id type = type_id<T>::value;
			destroy(type, handle);
		}

	private:
		const cache_storage& get_storage(string_id type) const;
		cache_storage&		 get_storage(string_id type);

#ifdef SFG_TOOLMODE
		struct resource_watch
		{
			string			path		 = "";
			string_id		type_id		 = 0;
			resource_handle base_handle	 = {};
			vector<string>	dependencies = {};
			string			root_dir	 = "";
		};
		void		add_resource_watch(resource_handle base_handle, const char* relative_path, const vector<string>& dependencies, string_id type, const char* root_dir);
		static void on_watched_resource_modified(const char* path, u64 last_modified, u16 id, void* user_data);
#endif

	private:
#ifdef SFG_TOOLMODE
		simple_file_watcher	   _file_watch = {};
		vector<resource_watch> _watched_resources;
#endif

		chunk_allocator32	  _aux_memory = {};
		vector<cache_storage> _storages	  = {};
		world&				  _world;
		resource_handle		  _dummy_color_texture	   = {};
		resource_handle		  _dummy_orm_texture	   = {};
		resource_handle		  _dummy_normal_texture	   = {};
		resource_handle		  _default_gbuffer_shader  = {};
		resource_handle		  _default_forward_shader  = {};
		resource_handle		  _default_gui_shader	   = {};
		resource_handle		  _default_gui_text_shader = {};
		resource_handle		  _default_gui_sdf_shader  = {};
		resource_handle		  _default_gui_mat		   = {};
		resource_handle		  _default_gui_text_mat	   = {};
		resource_handle		  _default_gui_sdf_mat	   = {};
		u32					  _max_load_priority	   = 0;
		u32					  _dynamic_sampler_count   = 0;

		// raws for defaults
		texture_raw _dummy_color_raw   = {};
		texture_raw _dummy_orm_raw	   = {};
		texture_raw _dummy_normal_raw  = {};
		u8*			_dummy_color_data  = nullptr;
		u8*			_dummy_normal_data = nullptr;
		u8*			_dummy_orm_data	   = nullptr;
	};
}
