// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/common/string_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

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
		void uninit();

	private:
		resource_manager_t* _mgr = nullptr;
		vector_t<sid_t>		_loaded;
	};
}
