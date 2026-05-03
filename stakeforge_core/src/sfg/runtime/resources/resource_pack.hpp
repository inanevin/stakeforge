// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "resource_cooker.hpp"
#include <sfg/common/string_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

#if !defined(SFG_EMBED_ASSETS)
#include <sfg/io/simple_file_watcher.hpp>
#endif

namespace sfg
{
	class resource_manager_t;

	struct embedded_resource_t
	{
		const char*		path = nullptr;
		const u8*		data = nullptr;
		u32				size = 0;
		resource_type_e type = resource_type_e::invalid;
	};

	class resource_pack_t
	{
	public:
		struct init_params_t
		{
			string_t				   manifest_path;
			string_t				   assets_dir;
			string_t				   cache_dir;
			const embedded_resource_t* embedded_entries		= nullptr;
			u32						   embedded_entry_count = 0;
		};

		bool init(resource_manager_t& mgr, const init_params_t& params);
		void tick();
		void uninit();

	private:
#if !defined(SFG_EMBED_ASSETS)
		struct watched_entry_t
		{
			string_t		  source_path;
			string_t		  name;
			cooking_options_t options;
			resource_type_e	  type = resource_type_e::invalid;
			sid_t			  sid  = 0;
		};

		static void on_file_changed(const char* path, u64 last_modified, u16 id, void* user_data);
		void		recook_watched(u16 id);
#endif

	private:
		resource_manager_t* _mgr = nullptr;
		vector_t<sid_t>		_loaded;

#if !defined(SFG_EMBED_ASSETS)
		string_t				  _cache_dir;
		simple_file_watcher_t	  _watcher;
		vector_t<watched_entry_t> _watched;
#endif
	};
}
