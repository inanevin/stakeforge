// Copyright (c) 2025 Inan Evin

#include "engine_runtime.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/engine/freetype_runtime.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

namespace sfg
{
	namespace
	{
		resource_file_system_t g_default_resource_file_system;
	}

	void engine_runtime_t::init_globals(size_t resource_manager_memory)
	{
		init_globals(g_default_resource_file_system, resource_manager_memory);
	}

	void engine_runtime_t::init_globals(resource_file_system_t& resource_file_system, size_t resource_manager_memory)
	{
		g_engine_thread_ids.main_thread_id = SFG_THIS_THREAD_ID();
		job_system_t::get().init();
		time_t::init();
		process::init();
		freetype_runtime_t::init();
		resource_manager_t::get().init(resource_file_system, resource_manager_memory);
	}

	void engine_runtime_t::uninit_globals()
	{
		job_system_t::get().wait_for_all();
		resource_manager_t::get().uninit();
		freetype_runtime_t::uninit();
		job_system_t::get().uninit();
		time_t::uninit();
		process::uninit();
	}

	bool engine_runtime_t::init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config = {})
	{
		if (!gfx_backend::get().init())
			return false;

		render_globals_t::s_global_bind_layout		   = gfx_util_t::create_bind_layout_global(false);
		render_globals_t::s_global_compute_bind_layout = gfx_util_t::create_bind_layout_global(true);
		resource_manager_t::get().init_atlases(glyph_atlas_config);
		render_resources_t::get().init();

		return true;
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

	void engine_runtime_t::update_settings(const engine_runtime_settings_t& settings)
	{
		_settings							   = settings;
		_settings.world_tick_rate			   = math::clamp(_settings.world_tick_rate, 15u, 240u);
		_settings.world_physics_rate		   = math::clamp(_settings.world_physics_rate, 30u, 240u);
		_settings.max_sim_steps				   = math::clamp(_settings.max_sim_steps, 1u, 8u);
		_settings.shadows.min_resolution	   = math::clamp<u16>(_settings.shadows.min_resolution, 64, 8192);
		_settings.shadows.max_resolution	   = math::clamp<u16>(_settings.shadows.max_resolution, _settings.shadows.min_resolution, 8192);
		_settings.shadows.max_views			   = math::min<u16>(_settings.shadows.max_views, ENGINE_SHADOW_VIEW_MAX);
		_settings.shadows.shadow_distance	   = math::max(_settings.shadows.shadow_distance, 1.0f);
		_settings.shadows.shadow_fade_distance = math::clamp(_settings.shadows.shadow_fade_distance, 0.0f, _settings.shadows.shadow_distance);
	}

}
