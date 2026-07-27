// Copyright (c) 2025 Inan Evin

#include "engine_runtime.hpp"
#include "engine_runtime_config.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/physics/physics_runtime.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/engine/freetype_runtime.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

namespace sfg
{
	engine_runtime_t& engine_runtime_t::get()
	{
		static engine_runtime_t s_instance;
		return s_instance;
	}

	void engine_runtime_t::init_globals()
	{
		init_globals(engine_global_config_t{});
	}

	void engine_runtime_t::init_globals(size_t resource_manager_memory)
	{
		init_globals(_resource_file_system, resource_manager_memory);
	}

	void engine_runtime_t::init_globals(resource_file_system_t& resource_file_system, size_t resource_manager_memory)
	{
		init_globals(resource_file_system, {.resource_manager = {.memory_budget_bytes = resource_manager_memory}});
	}

	void engine_runtime_t::init_globals(const engine_global_config_t& config)
	{
		init_globals(_resource_file_system, config);
	}

	void engine_runtime_t::init_globals(resource_file_system_t& resource_file_system, const engine_global_config_t& config)
	{
		g_engine_thread_ids.main_thread_id = SFG_THIS_THREAD_ID();
		job_system_t::get().init(config.job_worker_count);
		physics_runtime_t::init();
		time_t::init();
		process::init();
		freetype_runtime_t::init();
		resource_manager_t::get().init(resource_file_system, config.resource_manager);
	}

	void engine_runtime_t::uninit_globals()
	{
		job_system_t::get().wait_for_all();
		resource_manager_t::get().uninit();
		freetype_runtime_t::uninit();
		physics_runtime_t::uninit();
		job_system_t::get().uninit();
		time_t::uninit();
		process::uninit();
	}

	bool engine_runtime_t::init_backend()
	{
		return init_backend(engine_backend_config_t{});
	}

	bool engine_runtime_t::init_backend(const engine_backend_config_t& config)
	{
		if (!gfx_backend::get().init(config.gfx))
			return false;

		render_globals_t::s_global_bind_layout		   = gfx_util_t::create_bind_layout_global(false);
		render_globals_t::s_global_compute_bind_layout = gfx_util_t::create_bind_layout_global(true);
		resource_manager_t::get().init_atlases(config.glyph_atlas);
		render_resources_t::get().init(config.render_resources);

		return true;
	}

	bool engine_runtime_t::init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config)
	{
		return init_backend(engine_backend_config_t{.glyph_atlas = glyph_atlas_config});
	}

	void engine_runtime_t::uninit_backend()
	{
		resource_manager_t::get().uninit_atlases();
		render_resources_t::get().uninit();
		gfx_backend& backend = gfx_backend::get();
		if (!render_globals_t::s_global_bind_layout.is_null())
		{
			backend.destroy_bind_layout(render_globals_t::s_global_bind_layout);
			render_globals_t::s_global_bind_layout = {};
		}
		if (!render_globals_t::s_global_compute_bind_layout.is_null())
		{
			backend.destroy_bind_layout(render_globals_t::s_global_compute_bind_layout);
			render_globals_t::s_global_compute_bind_layout = {};
		}
		backend.uninit();
		g_engine_thread_ids.main_thread_id = 0;
	}

	bool engine_runtime_t::init()
	{
		SFG_INFO("engine runtime initialized correctly.");
		return true;
	}

	void engine_runtime_t::uninit()
	{
	}

	void engine_runtime_t::update_project_settings(const project_settings_t& settings)
	{
		_project_settings = settings;
		_project_settings.normalize();
	}

}
