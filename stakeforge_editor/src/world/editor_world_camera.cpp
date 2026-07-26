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

#include "world/editor_world_camera.hpp"

#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
#define EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION		0.18f
#define EDITOR_WORLD_CAMERA_FOCUS_MAX_DURATION		0.65f
#define EDITOR_WORLD_CAMERA_FOCUS_DURATION_PER_UNIT 0.015f

	void editor_world_camera_t::begin_focus(world_t& world, const vec3f_t& target_position)
	{
		_focus_start_position  = world.get_entity_pos_local(get_entity());
		_focus_target_position = target_position;

		const f32 distance = vec3f_t::distance(_focus_start_position, _focus_target_position);

		_focus_elapsed	= 0.0f;
		_focus_duration = math::clamp(EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION + distance * EDITOR_WORLD_CAMERA_FOCUS_DURATION_PER_UNIT, EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION, EDITOR_WORLD_CAMERA_FOCUS_MAX_DURATION);
		_focus_enabled	= true;
	}

	void editor_world_camera_t::cancel_focus()
	{
		_focus_start_position  = vec3f_t::zero;
		_focus_target_position = vec3f_t::zero;
		_focus_elapsed		   = 0.0f;
		_focus_duration		   = 0.0f;
		_focus_enabled		   = false;
	}

	bool editor_world_camera_t::tick_focus(world_t& world, f32 dt_seconds)
	{
		if (!_focus_enabled)
			return false;

		_focus_elapsed = math::min(_focus_elapsed + dt_seconds, _focus_duration);

		const f32	  alpha		  = _focus_elapsed / _focus_duration;
		const f32	  eased_alpha = easing_t::ease_in_out(0.0f, 1.0f, alpha);
		const vec3f_t position	  = vec3f_t::lerp(_focus_start_position, _focus_target_position, eased_alpha);

		world.set_entity_pos_local(get_entity(), position);

		if (_focus_elapsed >= _focus_duration)
			_focus_enabled = false;

		return true;
	}
}
