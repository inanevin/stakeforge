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
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY 0.2f
#define EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE		0.1f
#define EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE		1000.0f
#define EDITOR_WORLD_CAMERA_ORBIT_WHEEL_STEP		0.15f
#define EDITOR_WORLD_CAMERA_ORBIT_ZOOM_SMOOTH_TIME	0.12f
#define EDITOR_WORLD_CAMERA_ORBIT_ZOOM_MAX_SPEED	1000.0f

	void editor_world_camera_orbit_t::init(world_t& world)
	{
		_camera_entity = world.create_entity("editor camera");

		component_camera_t& camera = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		camera.priority			   = -1;
		camera.near_plane		   = 0.01f;

		ecs_helpers_t::table_add_or_get_as<component_post_process_t>(world.get_component_table(type_id_t<component_post_process_t>::value), _camera_entity);
		ecs_t::table_add(world.get_component_table(type_id_t<component_no_serialize_t>::value), _camera_entity);

		apply_transform(world);
	}

	void editor_world_camera_orbit_t::uninit(world_t& world)
	{
		cancel_focus();
		world.destroy_entity(_camera_entity);

		_camera_entity		  = NULL_ENTITY_ID;
		_target				  = vec3f_t::zero;
		_mouse_delta		  = vec2f_t::zero;
		_camera_yaw_degrees	  = 0.0f;
		_camera_pitch_degrees = 25.0f;
		_distance			  = 8.0f;
		_distance_target	  = 8.0f;
		_distance_velocity	  = 0.0f;
	}

	void editor_world_camera_orbit_t::pass_input(world_t& world, const editor_world_camera_input_t& input)
	{
		if (is_focus_enabled())
		{
			const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);

			_target			   = world.get_entity_pos_local(_camera_entity) + rotation.get_forward() * _distance;
			_distance_target   = _distance;
			_distance_velocity = 0.0f;
		}

		cancel_focus();

		if (input.reset)
		{
			_mouse_delta	   = vec2f_t::zero;
			_distance_target   = _distance;
			_distance_velocity = 0.0f;
			return;
		}

		_mouse_delta += input.mouse_delta;
		if (input.wheel_delta != 0.0f)
			_distance_target = math::clamp(_distance_target - input.wheel_delta * _distance_target * EDITOR_WORLD_CAMERA_ORBIT_WHEEL_STEP, EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE, EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE);
	}

	void editor_world_camera_orbit_t::fit_to_bounds(world_t& world, const aabb_t& bounds)
	{
		const component_camera_t& camera   = ecs_helpers_t::table_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		const vec3f_t			  half	   = bounds.bounds_half_extent;
		const f32				  rad	   = math::max(half.x, math::max(half.y, half.z));
		const quat_t			  rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);

		_target			   = (bounds.bounds_min + bounds.bounds_max) * 0.5f;
		_distance		   = math::clamp(rad / math::sin(DEG_2_RAD * camera.fov_degrees * 0.5f), EDITOR_WORLD_CAMERA_ORBIT_MIN_DISTANCE, EDITOR_WORLD_CAMERA_ORBIT_MAX_DISTANCE);
		_distance_target   = _distance;
		_distance_velocity = 0.0f;
		_mouse_delta	   = vec2f_t::zero;
		begin_focus(world, _target - rotation.get_forward() * _distance);
	}

	void editor_world_camera_orbit_t::tick(world_t& world, f32 dt_seconds)
	{
		if (tick_focus(world, dt_seconds))
			return;

		_camera_yaw_degrees -= _mouse_delta.x * EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY;
		_camera_pitch_degrees -= _mouse_delta.y * EDITOR_WORLD_CAMERA_ORBIT_MOUSE_SENSITIVITY;
		_camera_pitch_degrees = math::clamp(_camera_pitch_degrees, -89.0f, 89.0f);
		_mouse_delta		  = vec2f_t::zero;

		_distance = easing_t::smooth_damp(_distance, _distance_target, &_distance_velocity, EDITOR_WORLD_CAMERA_ORBIT_ZOOM_SMOOTH_TIME, EDITOR_WORLD_CAMERA_ORBIT_ZOOM_MAX_SPEED, dt_seconds);

		apply_transform(world);
	}

	void editor_world_camera_orbit_t::serialize(const world_t& world, nlohmann::json& out_json) const
	{
		const vec3f_t& position = world.get_entity_pos_local(_camera_entity);
		const quat_t&  rotation = world.get_entity_rot_local(_camera_entity);
		out_json				= {
			{"position", {{"x", position.x}, {"y", position.y}, {"z", position.z}}},
			{"rotation", {{"x", rotation.x}, {"y", rotation.y}, {"z", rotation.z}, {"w", rotation.w}}},
			{"target", {{"x", _target.x}, {"y", _target.y}, {"z", _target.z}}},
			{"yaw_degrees", _camera_yaw_degrees},
			{"pitch_degrees", _camera_pitch_degrees},
			{"distance", _distance},
			{"distance_target", _distance_target},
			{"distance_velocity", _distance_velocity},
		};
	}

	void editor_world_camera_orbit_t::deserialize(world_t& world, const nlohmann::json& in_json)
	{
		cancel_focus();

		vec3f_t position = world.get_entity_pos_local(_camera_entity);
		quat_t	rotation = world.get_entity_rot_local(_camera_entity);

		const nlohmann::json position_json = in_json.value<nlohmann::json>("position", nlohmann::json::object());
		const nlohmann::json rotation_json = in_json.value<nlohmann::json>("rotation", nlohmann::json::object());
		position.x						   = position_json.value<f32>("x", position.x);
		position.y						   = position_json.value<f32>("y", position.y);
		position.z						   = position_json.value<f32>("z", position.z);
		rotation.x						   = rotation_json.value<f32>("x", rotation.x);
		rotation.y						   = rotation_json.value<f32>("y", rotation.y);
		rotation.z						   = rotation_json.value<f32>("z", rotation.z);
		rotation.w						   = rotation_json.value<f32>("w", rotation.w);

		const vec3f_t euler	  = quat_t::to_euler(rotation);
		_camera_yaw_degrees	  = in_json.value<f32>("yaw_degrees", euler.y);
		_camera_pitch_degrees = in_json.value<f32>("pitch_degrees", euler.x);
		_distance			  = in_json.value<f32>("distance", _distance);
		_distance_target	  = in_json.value<f32>("distance_target", _distance);
		_distance_velocity	  = in_json.value<f32>("distance_velocity", 0.0f);
		_target				  = position + rotation.get_forward() * _distance;

		const nlohmann::json target_json = in_json.value<nlohmann::json>("target", nlohmann::json::object());
		_target.x						 = target_json.value<f32>("x", _target.x);
		_target.y						 = target_json.value<f32>("y", _target.y);
		_target.z						 = target_json.value<f32>("z", _target.z);
		_mouse_delta					 = vec2f_t::zero;
		apply_transform(world);
	}

	void editor_world_camera_orbit_t::apply_transform(world_t& world)
	{
		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		world.set_entity_rot_local(_camera_entity, rotation);
		world.set_entity_pos_local(_camera_entity, _target - rotation.get_forward() * _distance);
	}
}
