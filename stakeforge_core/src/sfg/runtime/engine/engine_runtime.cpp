// Copyright (c) 2025 Inan Evin

#include "engine_runtime.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/log.hpp>
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
		resource_manager_t::get().uninit();
		freetype_runtime_t::uninit();
		job_system_t::get().uninit();
		time_t::uninit();
		process::uninit();
		g_engine_thread_ids.main_thread_id = 0;
	}

	bool engine_runtime_t::init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config = {})
	{
		if (!gfx_backend::get().init())
			return false;

		render_globals_t::s_global_bind_layout = gfx_util_t::create_bind_layout_global(false);
		resource_manager_t::get().init_atlases(glyph_atlas_config);
		render_resources_t::get().get_texture_upload_queue().init();

		return true;
	}

	void engine_runtime_t::uninit_backend()
	{
		render_resources_t::get().drain_requests();
		render_resources_t::get().get_texture_upload_queue().uninit();
		gfx_backend& backend = gfx_backend::get();
		if (!render_globals_t::s_global_bind_layout.is_null())
		{
			backend.destroy_bind_layout(render_globals_t::s_global_bind_layout);
			render_globals_t::s_global_bind_layout = {};
		}
		backend.uninit();
	}

	bool engine_runtime_t::init()
	{
		SFG_INFO("engine runtime initialized correctly.");
		return true;
	}

	void engine_runtime_t::uninit()
	{
		for (auto it = _worlds.begin_handle(); it != _worlds.end_handle();)
		{
			const world_handle_t handle = *it;
			++it;

			world_t& world = _worlds.get(handle);
			world.uninit();
			_worlds.remove(handle);
		}
	}

	world_handle_t engine_runtime_t::create_world()
	{
		const world_handle_t handle = _worlds.add();
		world_t&			 world	= _worlds.get(handle);
		world.init();
		return handle;
	}

	bool engine_runtime_t::destroy_world(world_handle_t handle)
	{
		if (!_worlds.is_valid(handle))
			return false;

		world_t& world = _worlds.get(handle);
		world.uninit();
		_worlds.remove(handle);
		return true;
	}

	bool engine_runtime_t::is_world_valid(world_handle_t handle) const
	{
		return _worlds.is_valid(handle);
	}

	world_t& engine_runtime_t::get_world(world_handle_t handle)
	{
		return _worlds.get(handle);
	}

	const world_t& engine_runtime_t::get_world(world_handle_t handle) const
	{
		return _worlds.get(handle);
	}

	resource_file_system_t& engine_runtime_t::get_resource_file_system()
	{
		return _resource_file_system;
	}
}
