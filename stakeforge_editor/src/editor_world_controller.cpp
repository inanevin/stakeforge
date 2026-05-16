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

#include "editor_world_controller.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	void editor_world_controller_t::init(engine_runtime_t& runtime)
	{
		_runtime		  = &runtime;
		_previous_time_us = time_t::get_cpu_microseconds();
		_accumulator_us	  = 0;
		_alpha			  = 0.0f;
	}

	void editor_world_controller_t::uninit()
	{
		destroy_worlds();
		_runtime		  = nullptr;
		_previous_time_us = 0;
		_accumulator_us	  = 0;
		_alpha			  = 0.0f;
	}

	world_handle_t editor_world_controller_t::create_world()
	{
		SFG_ASSERT(_runtime != nullptr);
		const world_handle_t handle = _runtime->create_world();
		_worlds.push_back(handle);
		return handle;
	}

	void editor_world_controller_t::destroy_world(world_handle_t handle)
	{
		SFG_ASSERT(_runtime != nullptr);
		for (auto it = _worlds.begin(); it != _worlds.end(); ++it)
		{
			if (*it == handle)
			{
				_worlds.erase(it);
				break;
			}
		}

		if (_main_world == handle)
			_main_world = {};

		_runtime->destroy_world(handle);
	}

	void editor_world_controller_t::destroy_worlds()
	{
		SFG_ASSERT(_runtime != nullptr);
		for (world_handle_t handle : _worlds)
			_runtime->destroy_world(handle);
		_worlds.clear();
		_main_world = {};
	}

	void editor_world_controller_t::tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps)
	{
		SFG_ASSERT(_runtime != nullptr);
		_world_physics_rate = world_physics_rate;

		const i64 now	   = time_t::get_cpu_microseconds();
		const i64 delta_us = now - _previous_time_us;
		_previous_time_us  = now;

		if (world_tick_rate == 0)
			return;

		const i64 fixed_us = 1000000 / static_cast<i64>(world_tick_rate);
		if (fixed_us == 0)
			return;

		_accumulator_us += delta_us;

		const f32 dt_seconds = 1.0f / static_cast<f32>(world_tick_rate);
		u32		  steps		 = 0;
		while (_accumulator_us >= fixed_us && steps < max_sim_steps)
		{
			_accumulator_us -= fixed_us;
			for (world_handle_t handle : _worlds)
				_runtime->get_world(handle).tick(dt_seconds);
			++steps;
		}

		_alpha = static_cast<f32>(static_cast<double>(_accumulator_us) / static_cast<double>(fixed_us));
	}

	void editor_world_controller_t::install_default_world(world_handle_t)
	{
	}

	void editor_world_controller_t::set_main_world(world_handle_t handle)
	{
		_main_world = handle;
	}

	world_handle_t editor_world_controller_t::get_main_world() const
	{
		return _main_world;
	}

	f32 editor_world_controller_t::get_alpha() const
	{
		return _alpha;
	}
}
