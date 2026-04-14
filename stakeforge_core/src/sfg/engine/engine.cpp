/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "engine.hpp"
#include "engine_config.hpp"
#include "engine_stats.hpp"
#include "platform/process.hpp"
#include "platform/time.hpp"
#include "world/world.hpp"

#include <functional>

namespace sfg
{
	namespace
	{
		constexpr engine_id_t INITIAL_WINDOWS = 32;
	}

	engine_error_code engine_t::init()
	{
		const double fixed_framerate_ns = g_engine_config.fixed_framerate_ns;

		const u8 renderer_result = _renderer.init();
		if (renderer_result != static_cast<u8>(engine_error_code::none))
			return static_cast<engine_error_code>(renderer_result);

		time_t::init();

		_previous_time	   = time_t::get_cpu_microseconds();
		_accumulator_ns	   = static_cast<i64>(fixed_framerate_ns);
		_start_time		   = _previous_time;
		_fps_main_time	   = _previous_time;
		_fps_render_time   = _previous_time;
		_fps_main_frames   = 0;
		_fps_render_frames = 0;
		_is_init		   = true;

		g_engine_stats.reset();
		g_engine_stats.main_thread_id = static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

		_windows.reserve(INITIAL_WINDOWS);

		SFG_INFO("engine initialized correctly.");
		return engine_error_code::none;
	}

	engine_error_code engine_t::init(const engine_config_t& config)
	{
		g_engine_config = config;
		return init();
	}

	void engine_t::uninit()
	{
		end_render();

		for (auto it = _windows.begin_handle(); it != _windows.end_handle();)
		{
			const window_handle_t handle = *it;
			++it;

			engine_window_t& window = _windows.get(handle);
			if (window.runtime.window_handle != nullptr)
				process::destroy_window(window.runtime.window_handle);

			_windows.remove(handle);
		}

		_renderer.shutdown();
		_previous_time	   = 0;
		_accumulator_ns	   = 0;
		_start_time		   = 0;
		_fps_main_time	   = 0;
		_fps_render_time   = 0;
		_fps_main_frames   = 0;
		_fps_render_frames = 0;
		_is_init		   = false;
		g_engine_stats.reset();
	}

	void engine_t::tick()
	{
		process::pump_os_messages();
		tick_windows();

		const i64	 tick_start				   = time_t::get_cpu_microseconds();
		const double fixed_framerate_ns_d	   = g_engine_config.fixed_framerate_ns;
		const i64	 fixed_framerate_ns		   = static_cast<i64>(fixed_framerate_ns_d);
		const f32	 fixed_framerate_s		   = static_cast<f32>(fixed_framerate_ns_d / 1'000'000'000.0);
		const u32	 fixed_framerate_max_ticks = g_engine_config.fixed_framerate_max_ticks;

		const i64 current_time = time_t::get_cpu_microseconds();
		const i64 delta_micro  = current_time - _previous_time;
		_previous_time		   = current_time;

		u32 ticks = 0;
		_accumulator_ns += delta_micro * 1000;

		while (_accumulator_ns >= fixed_framerate_ns && ticks < fixed_framerate_max_ticks)
		{
			_accumulator_ns -= fixed_framerate_ns;

			// for (world_t* w : _worlds)
			//	w->tick(fixed_framerate_s);

			ticks++;
		}

		const i64 tick_end						= time_t::get_cpu_microseconds();
		g_engine_stats.main_thread_time_ms		= static_cast<double>(tick_end - tick_start) / 1000.0;
		g_engine_stats.app_elapsed_time_seconds = static_cast<double>(tick_end - _start_time) / 1000000.0;

		_fps_main_frames++;

		const i64 fps_delta = tick_end - _fps_main_time;
		if (fps_delta >= 1000000)
		{
			g_engine_stats.fps_main = static_cast<u32>((static_cast<u64>(_fps_main_frames) * 1000000ull) / static_cast<u64>(fps_delta));
			_fps_main_time			= tick_end;
			_fps_main_frames		= 0;
		}
	}

	void engine_t::start_render()
	{
		if (_render_thread_active)
			return;

		_render_thread_active = true;
		_render_thread		  = std::thread(&engine_t::render, this);
	}

	void engine_t::end_render()
	{
		if (!_render_thread_active && !_render_thread.joinable())
			return;

		_render_thread_active = false;

		if (_render_thread.joinable())
			_render_thread.join();

		g_engine_stats.render_thread_id = 0;
	}

	window_handle_t engine_t::create_window(const window_descriptor_t& descriptor)
	{
		const window_handle_t handle			= _windows.add();
		engine_window_t&	  window			= _windows.get(handle);
		window.descriptor						= descriptor;
		window.runtime							= {};
		window.runtime.event_callback			= &engine_t::on_window_event;
		window.runtime.event_callback_user_data = this;
		window.runtime.high_frequency_input		= descriptor.high_frequency_input;

		for (auto it = _windows.begin_handle(); it != _windows.end_handle(); ++it)
		{
			engine_window_t& window = _windows.get(*it);
			if (window.runtime.window_handle != nullptr)
				process::set_window_runtime(window.runtime.window_handle, window.runtime);
		}

		if (!process::create_window(descriptor.title, descriptor.pos, descriptor.size, descriptor.style, window.runtime))
		{
			SFG_ERR("failed creating window!");
			_windows.remove(handle);
			return {};
		}

		return handle;
	}

	void engine_t::destroy_window(window_handle_t handle)
	{
		engine_window_t& window = _windows.get(handle);
		if (window.runtime.window_handle != nullptr)
			process::destroy_window(window.runtime.window_handle);

		_windows.remove(handle);
	}

	bool engine_t::is_window_valid(window_handle_t handle) const
	{
		return _windows.is_valid(handle);
	}

	window_descriptor_t& engine_t::get_window_descriptor(window_handle_t handle)
	{
		return _windows.get(handle).descriptor;
	}

	const window_descriptor_t& engine_t::get_window_descriptor(window_handle_t handle) const
	{
		return _windows.get(handle).descriptor;
	}

	const window_runtime_t& engine_t::get_window_runtime(window_handle_t handle) const
	{
		return _windows.get(handle).runtime;
	}

	void engine_t::tick_windows()
	{
		for (auto it = _windows.begin_handle(); it != _windows.end_handle();)
		{
			const window_handle_t handle = *it;
			++it;

			engine_window_t&  window  = _windows.get(handle);
			window_runtime_t& runtime = window.runtime;

			if (runtime.close_requested)
			{
				if (runtime.window_handle != nullptr)
					process::destroy_window(runtime.window_handle);

				_windows.remove(handle);
				continue;
			}

			window_descriptor_t& descriptor = window.descriptor;
			runtime.high_frequency_input	= descriptor.high_frequency_input;

			if (descriptor.pos != runtime.pos)
				process::set_window_position(runtime.window_handle, descriptor.pos);

			if (descriptor.size != runtime.size)
				process::set_window_size(runtime.window_handle, descriptor.size, descriptor.style);

			if (descriptor.style != runtime.style)
			{
				process::set_window_style(runtime.window_handle, descriptor.size, descriptor.style);
				runtime.style = descriptor.style;
			}
		}
	}


	void engine_t::render()
	{
		g_engine_stats.render_thread_id = static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

		while (_render_thread_active)
		{
			const i64 render_start = time_t::get_cpu_microseconds();
			_renderer.render();
			const i64 render_end				 = time_t::get_cpu_microseconds();
			g_engine_stats.render_thread_time_ms = static_cast<double>(render_end - render_start) / 1000.0;
			g_engine_stats.render_frame_counter++;
			_fps_render_frames++;

			const i64 fps_delta = render_end - _fps_render_time;
			if (fps_delta >= 1000000)
			{
				g_engine_stats.fps_render = static_cast<u32>((static_cast<u64>(_fps_render_frames) * 1000000ull) / static_cast<u64>(fps_delta));
				_fps_render_time		  = render_end;
				_fps_render_frames		  = 0;
			}
		}
	}

	void engine_t::on_window_event(void* handle, const window_event_t& ev, void* user_data)
	{
		engine_t* self = static_cast<engine_t*>(user_data);
		for (auto it = self->_windows.begin_handle(); it != self->_windows.end_handle(); ++it)
		{
			const window_handle_t window_handle = *it;
			engine_window_t&	  window		= self->_windows.get(window_handle);
			if (window.runtime.window_handle != handle)
				continue;

			if (ev.type == window_event_type_t::resize)
				window.descriptor.size = window.runtime.size;
			else if (ev.type == window_event_type_t::repos)
				window.descriptor.pos = window.runtime.pos;

			return;
		}
	}

}
