// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/resources/resource_file_system.hpp>

namespace sfg
{
	namespace ui
	{
		struct glyph_atlas_config_t;
	}

	class engine_runtime_t
	{
	public:
		static void init_globals(size_t resource_manager_memory = 64ull * 1024ull * 1024ull);
		static void init_globals(resource_file_system_t& resource_file_system, size_t resource_manager_memory = 64ull * 1024ull * 1024ull);
		static void uninit_globals();
		static bool init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config);
		static void uninit_backend();

		bool init();
		void uninit();

		resource_file_system_t& get_resource_file_system();

	private:
		resource_file_system_t _resource_file_system;
	};
}
