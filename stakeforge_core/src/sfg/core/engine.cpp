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
#include "platform/time.hpp"
#include "world/world.hpp"

namespace sfg
{
	void engine_t::init()
	{
		const double fixed_framerate_ns = g_engine_config.fixed_framerate_ns;

		if (!_renderer.init())
			return;

		_previous_time	= time_t::get_cpu_microseconds();
		_accumulator_ns = static_cast<i64>(fixed_framerate_ns);
	}

	void engine_t::uninit()
	{
		end_render();

		for (world_t* w : _worlds)
		{
			w->uninit();
			delete w;
		}

		_worlds.clear();
		_renderer.shutdown();
		_previous_time	= 0;
		_accumulator_ns = 0;
	}

	void engine_t::tick()
	{
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

			for (world_t* w : _worlds)
				w->tick(fixed_framerate_s);

			ticks++;
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
	}

	void engine_t::render()
	{
		while (_render_thread_active)
			time_t::yield_thread();
	}

	world_t* engine_t::create_world()
	{
		world_t* w = new world_t();
		w->init();
		_worlds.push_back(w);
		return w;
	}

	void engine_t::destroy_world(world_t* w)
	{
		for (auto it = _worlds.begin(); it != _worlds.end(); ++it)
		{
			if (*it != w)
				continue;

			w->uninit();
			delete w;
			_worlds.erase(it);
			return;
		}
	}

}
