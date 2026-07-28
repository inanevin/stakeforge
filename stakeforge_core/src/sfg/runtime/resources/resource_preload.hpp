// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/simple_file_watcher.hpp>

namespace sfg
{
	class resource_manager_t;

	class resource_preload_t final
	{
	public:
		resource_preload_t()									 = default;
		~resource_preload_t()									 = default;
		resource_preload_t(const resource_preload_t&)			 = delete;
		resource_preload_t& operator=(const resource_preload_t&) = delete;

		struct init_params_t
		{
			string_t manifest_path;
			string_t assets_dir;
			string_t cache_dir;
		};

		bool init(resource_manager_t& mgr, const init_params_t& params);
		void tick();
		void uninit();

	private:
		struct watched_entry_t
		{
			string_t		source_path;
			string_t		config_json;
			resource_type_e type = resource_type_e::invalid;
			sid_t			sid	 = 0;
		};

		static void on_file_changed(const char* path, u64 last_modified, u16 id, void* user_data);
		void		recook_watched(u16 id);

	private:
		resource_manager_t*		  _mgr = nullptr;
		vector_t<sid_t>			  _loaded;
		string_t				  _cache_dir;
		simple_file_watcher_t	  _watcher;
		vector_t<watched_entry_t> _watched;
	};
}
