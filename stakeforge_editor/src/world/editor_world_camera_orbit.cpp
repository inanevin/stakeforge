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

#include "world/editor_world_camera_orbit.hpp"
#include <sfg/math/aabb.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
#define EDITOR_WORLD_CAMERA_ORBIT_BASE_MOVE_SPEED	12.0f
#define EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY 0.2f
#define EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE		0.1f
#define EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE		1000.0f
#define EDITOR_WORLD_CAMERA_ORBIT_WHEEL_STEP		0.15f
#define EDITOR_WORLD_CAMERA_ORBIT_ZOOM_SMOOTH_TIME	0.12f
#define EDITOR_WORLD_CAMERA_ORBIT_ZOOM_MAX_SPEED	1000.0f

	void editor_world_camera_orbit_t::init(world_t& world)
	{
		_camera_entity			   = world.create_entity("editor camera");
		component_camera_t& camera = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		camera.priority			   = -1;
		camera.near_plane		   = 0.01f;
		ecs_t::table_add(world.get_component_table(type_id_t<component_no_serialize_t>::value), _camera_entity);

		apply_transform(world);
		_current_move_speed = EDITOR_WORLD_CAMERA_ORBIT_BASE_MOVE_SPEED;
	}

	void editor_world_camera_orbit_t::uninit(world_t& world)
	{
		world.destroy_entity(_camera_entity);
		_camera_entity		  = NULL_ENTITY_ID;
		_direction_input	  = vec3f_t::zero;
		_target				  = vec3f_t::zero;
		_mouse_delta		  = vec2f_t::zero;
		_camera_yaw_degrees	  = 0.0f;
		_camera_pitch_degrees = 25.0f;
		_distance			  = 8.0f;
		_distance_target	  = 8.0f;
		_distance_velocity	  = 0.0f;
		_current_move_speed	  = EDITOR_WORLD_CAMERA_ORBIT_BASE_MOVE_SPEED;
	}

	void editor_world_camera_orbit_t::pass_input(const editor_world_camera_input_t& input)
	{
		if (input.reset)
		{
			_direction_input	= vec3f_t::zero;
			_mouse_delta		= vec2f_t::zero;
			_distance_target	= _distance;
			_distance_velocity	= 0.0f;
			_current_move_speed = EDITOR_WORLD_CAMERA_ORBIT_BASE_MOVE_SPEED;
			return;
		}

		_direction_input += input.direction_delta;
		_mouse_delta += input.mouse_delta;
		if (input.set_move_speed)
			_current_move_speed = input.move_speed;
		if (input.wheel_delta != 0.0f)
			_distance_target = math::clamp(_distance_target - input.wheel_delta * _distance_target * EDITOR_WORLD_CAMERA_ORBIT_WHEEL_STEP, EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE, EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE);
	}

	void editor_world_camera_orbit_t::fit_to_bounds(world_t& world, const aabb_t& bounds)
	{
		const component_camera_t& camera = ecs_helpers_t::table_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		const vec3f_t			  half	 = bounds.bounds_half_extent;
		const f32				  rad	 = math::max(half.x, math::max(half.y, half.z));
		_target							 = (bounds.bounds_min + bounds.bounds_max) * 0.5f;
		_distance						 = math::clamp(rad / math::sin(DEG_2_RAD * camera.fov_degrees * 0.5f), EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE, EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE);
		_distance_target				 = _distance;
		_distance_velocity				 = 0.0f;
		_direction_input				 = vec3f_t::zero;
		_mouse_delta					 = vec2f_t::zero;
		apply_transform(world);
	}

	void editor_world_camera_orbit_t::tick(world_t& world, f32 dt_seconds)
	{
		_camera_yaw_degrees -= _mouse_delta.x * EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY;
		_camera_pitch_degrees -= _mouse_delta.y * EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY;
		_camera_pitch_degrees = math::clamp(_camera_pitch_degrees, -89.0f, 89.0f);
		_mouse_delta		  = vec2f_t::zero;

		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		_distance_target	  = math::clamp(_distance_target - _direction_input.z * _current_move_speed * dt_seconds, EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE, EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE);
		_distance			  = easing_t::smooth_damp(_distance, _distance_target, &_distance_velocity, EDITOR_WORLD_CAMERA_ORBIT_ZOOM_SMOOTH_TIME, EDITOR_WORLD_CAMERA_ORBIT_ZOOM_MAX_SPEED, dt_seconds);
		_target += (rotation.get_right() * _direction_input.x + rotation.get_up() * _direction_input.y) * (_current_move_speed * dt_seconds);

		apply_transform(world);
	}

	void editor_world_camera_orbit_t::apply_transform(world_t& world)
	{
		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		world.set_entity_rot_local(_camera_entity, rotation);
		world.set_entity_pos_local(_camera_entity, _target - rotation.get_forward() * _distance);
	}
}
