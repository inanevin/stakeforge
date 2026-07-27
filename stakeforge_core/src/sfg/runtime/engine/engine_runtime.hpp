// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/project/project_settings.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

namespace sfg
{
	namespace ui
	{
		struct glyph_atlas_config_t;
	}

	struct engine_global_config_t;
	struct engine_backend_config_t;

	class engine_runtime_t final
	{
	public:
		static engine_runtime_t& get();

		engine_runtime_t()									 = default;
		~engine_runtime_t()									 = default;
		engine_runtime_t(const engine_runtime_t&)			 = delete;
		engine_runtime_t& operator=(const engine_runtime_t&) = delete;

		void init_globals();
		void init_globals(size_t resource_manager_memory);
		void init_globals(resource_file_system_t& resource_file_system, size_t resource_manager_memory = 64ull * 1024ull * 1024ull);
		void init_globals(const engine_global_config_t& config);
		void init_globals(resource_file_system_t& resource_file_system, const engine_global_config_t& config);
		void uninit_globals();
		bool init_backend();
		bool init_backend(const engine_backend_config_t& config);
		bool init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config);
		void uninit_backend();

		bool init();
		void uninit();
		void update_project_settings(const project_settings_t& settings);

		inline resource_file_system_t& get_resource_file_system()
		{
			return _resource_file_system;
		}
		inline const project_settings_t& get_project_settings() const
		{
			return _project_settings;
		}

	private:
		resource_file_system_t _resource_file_system;
		project_settings_t	   _project_settings = {};
	};
}
