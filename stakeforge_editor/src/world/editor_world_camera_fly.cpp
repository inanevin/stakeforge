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

#include "world/editor_world_camera_fly.hpp"
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_CAMERA_FLY_BASE_MOVE_SPEED	  12.0f
#define EDITOR_WORLD_CAMERA_FLY_MOUSE_SENSITIVITY 0.2f

	void editor_world_camera_fly_t::init(world_t& world)
	{
		_camera_entity			   = world.create_entity("editor camera");
		component_camera_t& camera = ecs_helpers_t::table_add_or_get_as<component_camera_t>(world.get_component_table(type_id_t<component_camera_t>::value), _camera_entity);
		camera.priority			   = -1;
		camera.near_plane		   = 0.05f;
		ecs_helpers_t::table_add_or_get_as<component_post_process_t>(world.get_component_table(type_id_t<component_post_process_t>::value), _camera_entity);

		ecs_t::table_add(world.get_component_table(type_id_t<component_no_serialize_t>::value), _camera_entity);

		const vec3f_t euler	  = quat_t::to_euler(world.get_entity_rot_local(_camera_entity));
		_camera_pitch_degrees = euler.x;
		_camera_yaw_degrees	  = euler.y;
		_current_move_speed	  = EDITOR_WORLD_CAMERA_FLY_BASE_MOVE_SPEED;
	}

	void editor_world_camera_fly_t::uninit(world_t& world)
	{
		world.destroy_entity(_camera_entity);
		_camera_entity		  = NULL_ENTITY_ID;
		_direction_input	  = vec3f_t::zero;
		_mouse_delta		  = vec2f_t::zero;
		_camera_yaw_degrees	  = 0.0f;
		_camera_pitch_degrees = 0.0f;
		_current_move_speed	  = EDITOR_WORLD_CAMERA_FLY_BASE_MOVE_SPEED;
	}

	void editor_world_camera_fly_t::pass_input(const editor_world_camera_input_t& input)
	{
		if (input.reset)
		{
			_direction_input	= vec3f_t::zero;
			_mouse_delta		= vec2f_t::zero;
			_current_move_speed = EDITOR_WORLD_CAMERA_FLY_BASE_MOVE_SPEED;
			return;
		}

		_direction_input += input.direction_delta;
		_mouse_delta += input.mouse_delta;
		if (input.set_move_speed)
			_current_move_speed = input.move_speed;
	}

	void editor_world_camera_fly_t::tick(world_t& world, f32 dt_seconds)
	{
		_camera_yaw_degrees -= _mouse_delta.x * EDITOR_WORLD_CAMERA_FLY_MOUSE_SENSITIVITY;
		_camera_pitch_degrees -= _mouse_delta.y * EDITOR_WORLD_CAMERA_FLY_MOUSE_SENSITIVITY;
		_camera_pitch_degrees = math::clamp(_camera_pitch_degrees, -89.0f, 89.0f);
		_mouse_delta		  = vec2f_t::zero;

		const quat_t rotation = quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f);
		world.set_entity_rot_local(_camera_entity, rotation);

		vec3f_t move_dir = rotation.get_forward() * _direction_input.z + rotation.get_right() * _direction_input.x + vec3f_t::up * _direction_input.y;
		if (move_dir.is_zero())
			return;

		move_dir.normalize();
		const vec3f_t position = world.get_entity_pos_local(_camera_entity) + move_dir * (_current_move_speed * dt_seconds);
		world.set_entity_pos_local(_camera_entity, position);
	}

	void editor_world_camera_fly_t::serialize(const world_t& world, nlohmann::json& out_json) const
	{
		const vec3f_t& position = world.get_entity_pos_local(_camera_entity);
		const quat_t&  rotation = world.get_entity_rot_local(_camera_entity);

		out_json = {
			{"position", {{"x", position.x}, {"y", position.y}, {"z", position.z}}},
			{"rotation", {{"x", rotation.x}, {"y", rotation.y}, {"z", rotation.z}, {"w", rotation.w}}},
			{"yaw_degrees", _camera_yaw_degrees},
			{"pitch_degrees", _camera_pitch_degrees},
			{"move_speed", _current_move_speed},
		};
	}

	void editor_world_camera_fly_t::deserialize(world_t& world, const nlohmann::json& in_json)
	{
		vec3f_t position = world.get_entity_pos_local(_camera_entity);
		quat_t	rotation = world.get_entity_rot_local(_camera_entity);

		const nlohmann::json position_json = in_json.value<nlohmann::json>("position", nlohmann::json::object());
		const nlohmann::json rotation_json = in_json.value<nlohmann::json>("rotation", nlohmann::json::object());

		position.x = position_json.value<f32>("x", position.x);
		position.y = position_json.value<f32>("y", position.y);
		position.z = position_json.value<f32>("z", position.z);
		rotation.x = rotation_json.value<f32>("x", rotation.x);
		rotation.y = rotation_json.value<f32>("y", rotation.y);
		rotation.z = rotation_json.value<f32>("z", rotation.z);
		rotation.w = rotation_json.value<f32>("w", rotation.w);

		const vec3f_t euler	  = quat_t::to_euler(rotation);
		_camera_yaw_degrees	  = in_json.value<f32>("yaw_degrees", euler.y);
		_camera_pitch_degrees = in_json.value<f32>("pitch_degrees", euler.x);
		_current_move_speed	  = in_json.value<f32>("move_speed", EDITOR_WORLD_CAMERA_FLY_BASE_MOVE_SPEED);
		_direction_input	  = vec3f_t::zero;
		_mouse_delta		  = vec2f_t::zero;

		world.teleport_entity(_camera_entity, position, quat_t::from_euler(_camera_pitch_degrees, _camera_yaw_degrees, 0.0f), vec3f_t::one);
	}
}
