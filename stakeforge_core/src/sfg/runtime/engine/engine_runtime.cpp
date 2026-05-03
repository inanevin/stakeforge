// Copyright (c) 2025 Inan Evin

#include "engine_runtime.hpp"
#include "engine_stats.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/job/job_system.hpp>

#include <functional>

namespace sfg
{
#define THIS_THREAD_ID static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()))

	bool engine_runtime_t::init(const engine_config_t& config)
	{
		_config = config;
		job_system_t::get().init();

		gfx_backend* backend = gfx_backend::get();
		if (!backend)
		{
			SFG_ERR("engine runtime needs gfx_backend initialized!");
			return false;
		}

		_resource_manager.init(_config.resource_max_count, _config.resource_allocator_size);

		const double fixed_framerate_ns = _config.fixed_framerate_ns;

		_previous_time	   = time_t::get_cpu_microseconds();
		_accumulator_ns	   = static_cast<i64>(fixed_framerate_ns);
		_start_time		   = _previous_time;
		_fps_main_time	   = _previous_time;
		_fps_render_time   = _previous_time;
		_fps_main_frames   = 0;
		_fps_render_frames = 0;

		g_engine_runtime_stats.reset();
		g_engine_runtime_stats.main_thread_id = THIS_THREAD_ID;

		SFG_INFO("engine runtime initialized correctly.");
		return true;
	}

	void engine_runtime_t::uninit()
	{
		end_render();

		for (auto it = _worlds.begin_handle(); it != _worlds.end_handle();)
		{
			const world_handle_t handle = *it;
			++it;

			world_t& world = _worlds.get(handle);
			world.uninit();
			_worlds.remove(handle);
		}

		_resource_manager.uninit();
		_renderer.uninit();
		gfx_backend::uninit_instance();

		_previous_time	   = 0;
		_accumulator_ns	   = 0;
		_start_time		   = 0;
		_fps_main_time	   = 0;
		_fps_render_time   = 0;
		_fps_main_frames   = 0;
		_fps_render_frames = 0;
		g_engine_runtime_stats.reset();
		frame_allocator_tls_t::uninit();
		job_system_t::get().uninit();
	}

	void engine_runtime_t::tick()
	{
		SFG_ASSERT(THIS_THREAD_ID == g_engine_runtime_stats.main_thread_id);

		frame_allocator_tls_t::reset();
		ensure_render_thread();

		const i64	 tick_start				   = time_t::get_cpu_microseconds();
		const double fixed_framerate_ns_d	   = _config.fixed_framerate_ns;
		const i64	 fixed_framerate_ns		   = static_cast<i64>(fixed_framerate_ns_d);
		const f32	 fixed_framerate_s		   = static_cast<f32>(fixed_framerate_ns_d / 1'000'000'000.0);
		const u32	 fixed_framerate_max_ticks = _config.fixed_framerate_max_ticks;

		const i64 current_time = time_t::get_cpu_microseconds();
		const i64 delta_micro  = current_time - _previous_time;
		_previous_time		   = current_time;

		u32 ticks = 0;
		_accumulator_ns += delta_micro * 1000;

		while (_accumulator_ns >= fixed_framerate_ns && ticks < fixed_framerate_max_ticks)
		{
			_accumulator_ns -= fixed_framerate_ns;
			tick_worlds(fixed_framerate_s);
			ticks++;
		}

		const i64 tick_end								= time_t::get_cpu_microseconds();
		g_engine_runtime_stats.main_thread_time_ms		= static_cast<double>(tick_end - tick_start) / 1000.0;
		g_engine_runtime_stats.app_elapsed_time_seconds = static_cast<double>(tick_end - _start_time) / 1000000.0;

		_fps_main_frames++;

		const i64 fps_delta = tick_end - _fps_main_time;
		if (fps_delta >= 1000000)
		{
			g_engine_runtime_stats.fps_main = static_cast<u32>((static_cast<u64>(_fps_main_frames) * 1000000ull) / static_cast<u64>(fps_delta));
			_fps_main_time					= tick_end;
			_fps_main_frames				= 0;
		}
	}

	world_handle_t engine_runtime_t::create_world()
	{
		SFG_ASSERT(THIS_THREAD_ID == g_engine_runtime_stats.main_thread_id);

		const world_handle_t handle = _worlds.add();
		world_t&			 world	= _worlds.get(handle);
		world.init();
		return handle;
	}

	bool engine_runtime_t::destroy_world(world_handle_t handle)
	{
		SFG_ASSERT(THIS_THREAD_ID == g_engine_runtime_stats.main_thread_id);

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

	void engine_runtime_t::tick_worlds(f32 delta_time)
	{
		for (world_t& world : _worlds)
			world.tick(delta_time);
	}

	void engine_runtime_t::ensure_render_thread()
	{
		if (_render_thread_active)
			return;

		_render_thread_active = true;
		_render_thread		  = std::thread(&engine_runtime_t::render, this);
	}

	void engine_runtime_t::end_render()
	{
		if (!_render_thread_active && !_render_thread.joinable())
			return;

		_render_thread_active = false;

		if (_render_thread.joinable())
			_render_thread.join();

		_renderer.join();

		g_engine_runtime_stats.render_thread_id = 0;
	}

	void engine_runtime_t::render()
	{
		frame_allocator_tls_t::init(static_cast<size_t>(_config.render_frame_allocator_size));

		const u64 render_thread_id				= THIS_THREAD_ID;
		g_engine_runtime_stats.render_thread_id = render_thread_id;

		while (_render_thread_active)
		{
			frame_allocator_tls_t::reset();

			const i64 render_start = time_t::get_cpu_microseconds();
			_renderer.render();
			const i64 render_end						 = time_t::get_cpu_microseconds();
			g_engine_runtime_stats.render_thread_time_ms = static_cast<double>(render_end - render_start) / 1000.0;
			g_engine_runtime_stats.render_frame_counter++;
			_fps_render_frames++;

			const i64 fps_delta = render_end - _fps_render_time;
			if (fps_delta >= 1000000)
			{
				g_engine_runtime_stats.fps_render = static_cast<u32>((static_cast<u64>(_fps_render_frames) * 1000000ull) / static_cast<u64>(fps_delta));
				_fps_render_time				  = render_end;
				_fps_render_frames				  = 0;
			}

			time_t::yield_thread();
		}

		frame_allocator_tls_t::uninit();
	}

#undef THIS_THREAD_ID
}
