/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "world/editor_world_camera.hpp"

#include <sfg/math/aabb.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
#define EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION		0.18f
#define EDITOR_WORLD_CAMERA_FOCUS_MAX_DURATION		0.65f
#define EDITOR_WORLD_CAMERA_FOCUS_DURATION_PER_UNIT 0.015f
#define EDITOR_WORLD_CAMERA_FOCUS_DISTANCE_SCALE	0.7f

	void editor_world_camera_t::calculate_focus_target(const vec3f_t& camera_position, const quat_t& camera_rotation, const aabb_t& bounds, vec3f_t& target_position, quat_t& target_rotation)
	{
		const vec3f_t center  = (bounds.bounds_min + bounds.bounds_max) * 0.5f;
		const vec3f_t size	  = bounds.bounds_max - bounds.bounds_min;
		vec3f_t		  forward = center - camera_position;
		vec2f_t		  angles  = get_camera_angles(camera_rotation);

		if (forward.is_zero())
			forward = camera_rotation.get_forward();
		else
			forward.normalize();

		angles.x = math::radians_to_degrees(std::asin(math::clamp(forward.y, -1.0f, 1.0f)));

		if (forward.x * forward.x + forward.z * forward.z > MATH_EPS * MATH_EPS)
			angles.y = math::radians_to_degrees(std::atan2(-forward.x, -forward.z));

		target_rotation = quat_t::from_euler(angles.x, angles.y, 0.0f);

		const vec3f_t alignment	   = vec3f_t::abs(forward);
		const f32	  alignments[] = {alignment.x, alignment.y, alignment.z};
		const f32	  lengths[]	   = {size.x, size.y, size.z};
		u32			  axis		   = 0;

		for (u32 i = 1; i < 3; ++i)
		{
			if (alignments[i] < alignments[axis] || (alignments[i] == alignments[axis] && lengths[i] > lengths[axis]))
				axis = i;
		}

		const f32 distance = lengths[axis] * EDITOR_WORLD_CAMERA_FOCUS_DISTANCE_SCALE;

		target_position = center - target_rotation.get_forward() * distance;
	}

	vec2f_t editor_world_camera_t::get_camera_angles(const quat_t& rotation)
	{
		const vec3f_t forward = rotation.get_forward();
		const vec3f_t right	  = rotation.get_right();
		const f32	  pitch	  = math::radians_to_degrees(std::asin(math::clamp(forward.y, -1.0f, 1.0f)));
		const f32	  yaw	  = math::radians_to_degrees(forward.x * forward.x + forward.z * forward.z > MATH_EPS * MATH_EPS ? std::atan2(-forward.x, -forward.z) : std::atan2(-right.z, right.x));

		return {pitch, yaw};
	}

	void editor_world_camera_t::begin_focus(world_t& world, const vec3f_t& target_position, const quat_t& target_rotation)
	{
		_focus_start_rotation  = world.get_entity_rot_local(get_entity());
		_focus_target_rotation = target_rotation;
		_focus_start_position  = world.get_entity_pos_local(get_entity());
		_focus_target_position = target_position;

		const f32 distance = vec3f_t::distance(_focus_start_position, _focus_target_position);

		_focus_elapsed	= 0.0f;
		_focus_duration = math::clamp(EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION + distance * EDITOR_WORLD_CAMERA_FOCUS_DURATION_PER_UNIT, EDITOR_WORLD_CAMERA_FOCUS_MIN_DURATION, EDITOR_WORLD_CAMERA_FOCUS_MAX_DURATION);
		_focus_enabled	= true;
	}

	void editor_world_camera_t::cancel_focus()
	{
		_focus_start_rotation  = quat_t::identity;
		_focus_target_rotation = quat_t::identity;
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
		const quat_t  rotation	  = quat_t::slerp(_focus_start_rotation, _focus_target_rotation, eased_alpha);

		world.set_entity_pos_local(get_entity(), position);
		world.set_entity_rot_local(get_entity(), rotation);

		if (_focus_elapsed >= _focus_duration)
			_focus_enabled = false;

		return true;
	}
}
