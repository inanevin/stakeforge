// Copyright (c) 2025 Inan Evin

#include "engine_runtime.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	void engine_runtime_t::init_globals()
	{
		job_system_t::get().init();
		time_t::init();
		process::init();
		g_engine_thread_ids.main_thread_id = SFG_THIS_THREAD_ID();
	}

	void engine_runtime_t::uninit_globals()
	{
		job_system_t::get().uninit();
		time_t::uninit();
		process::uninit();
		g_engine_thread_ids.main_thread_id = 0;
	}

	bool engine_runtime_t::init_backend()
	{
		return gfx_backend::get().init();
	}

	void engine_runtime_t::uninit_backend()
	{
		render_resources_t::get().drain();
		gfx_backend::get().uninit();
	}

	bool engine_runtime_t::init()
	{
		if (!_renderer.init())
			return false;

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

		_renderer.uninit();
	}

	void engine_runtime_t::simulate(f32 delta_time)
	{
		for (world_t& world : _worlds)
			world.tick(delta_time);
	}

	void engine_runtime_t::render()
	{
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
}
