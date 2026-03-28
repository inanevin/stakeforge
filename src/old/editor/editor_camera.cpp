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

#include "editor_camera.hpp"
#include "editor/editor_settings.hpp"

#include "platform/window_common.hpp"
#include "platform/window.hpp"
#include "input/input_mappings.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"

#include "world/components/comp_camera.hpp"

namespace SFG
{
	void editor_camera::init(world& world, window& wnd)
	{
		_window = &wnd;
		_world	= &world;
	}

	void editor_camera::uninit()
	{
	}

	void editor_camera::activate()
	{
		SFG_ASSERT(_is_active == false);

		world&			   w  = *_world;
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		_entity = em.create_entity("editor_camera");

		_camera_trait = cm.add_component<comp_camera>(_entity);
		em.set_entity_transient(_entity, true);

		comp_camera& cam_trait = cm.get_component<comp_camera>(_camera_trait);
		cam_trait.set_values(w, 0.01f, 250.0f, 60.0f, {0.01f, 0.04f, 0.125f});
		cam_trait.set_main(w);

		em.set_entity_position(_entity, w.get_tool_camera_pos());
		em.set_entity_rotation(_entity, w.get_tool_camera_rot());

		const quat& rot = em.get_entity_rotation(_entity);

		const vector3 euler = quat::to_euler(rot);
		_yaw_degrees		= euler.y;
		_pitch_degrees		= euler.x;
		_is_active			= true;
	}

	void editor_camera::deactivate()
	{
		if (!_is_active)
			return;

		world&			   w  = *_world;
		entity_manager&	   em = w.get_entity_manager();
		component_manager& tm = w.get_comp_manager();

		em.destroy_entity(_entity);
		_entity		  = {};
		_camera_trait = {};

		reset_runtime();
		_is_active = false;
	}

	void editor_camera::set_is_looking(bool b)
	{
		if (b)
		{
			_window->confine_cursor(cursor_confinement::pointer);
			_window->set_cursor_visible(false);
			_is_looking = true;
		}
		else
		{
			_window->confine_cursor(cursor_confinement::none);
			_window->set_cursor_visible(true);
			_is_looking	 = false;
			_mouse_delta = vector2::zero;
		}
	}

	void editor_camera::reset_runtime()
	{
		_direction_input = vector3::zero;
		_mouse_delta	 = vector2::zero;
		_is_looking		 = false;
		_window->set_cursor_visible(true);
	}

	bool editor_camera::on_window_event(const window_event& ev)
	{
		if (_is_active == 0)
			return false;

		const u16 button = ev.button;

		switch (ev.type)
		{
		case window_event_type::focus: {
			reset_runtime();
			return false;
		}
		case window_event_type::key: {

			if (button == input_code::key_w && ev.sub_type == window_event_sub_type::press)
				_direction_input.z += 1.0f;
			else if (button == input_code::key_w && ev.sub_type == window_event_sub_type::release && _direction_input.z > 0.1f)
				_direction_input.z -= 1.0f;
			if (button == input_code::key_s && ev.sub_type == window_event_sub_type::press)
				_direction_input.z -= 1.0f;
			else if (button == input_code::key_s && ev.sub_type == window_event_sub_type::release && _direction_input.z < -0.1f)
				_direction_input.z += 1.0f;

			if (button == input_code::key_d && ev.sub_type == window_event_sub_type::press)
				_direction_input.x += 1.0f;
			else if (button == input_code::key_d && ev.sub_type == window_event_sub_type::release && _direction_input.x > 0.1f)
				_direction_input.x -= 1.0f;
			if (button == input_code::key_a && ev.sub_type == window_event_sub_type::press)
				_direction_input.x -= 1.0f;
			else if (button == input_code::key_a && ev.sub_type == window_event_sub_type::release && _direction_input.x < -0.1f)
				_direction_input.x += 1.0f;

			if (button == input_code::key_e && ev.sub_type == window_event_sub_type::press)
				_direction_input.y += 1.0f;
			else if (button == input_code::key_e && ev.sub_type == window_event_sub_type::release && _direction_input.y > 0.1f)
				_direction_input.y -= 1.0f;
			if (button == input_code::key_q && ev.sub_type == window_event_sub_type::press)
				_direction_input.y -= 1.0f;
			else if (button == input_code::key_q && ev.sub_type == window_event_sub_type::release && _direction_input.y < -0.1f)
				_direction_input.y += 1.0f;

			if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::press)
				_current_move_speed = _base_move_speed * _boost_multiplier;
			else if (button == input_code::key_lshift && ev.sub_type == window_event_sub_type::release)
				_current_move_speed = _base_move_speed;

			if (button == input_code::key_a || button == input_code::key_d || button == input_code::key_w || button == input_code::key_s || button == input_code::key_q || button == input_code::key_e || button == input_code::key_lshift)
				return true;
			break;
		}
		case window_event_type::delta: {
			if (_is_looking)
			{
				_mouse_delta.x += static_cast<f32>(ev.value.x);
				_mouse_delta.y += static_cast<f32>(ev.value.y);
				return true;
			}
			else
			{
				_mouse_delta = vector2::zero;
			}
		}
		default:
			break;
		}

		return false;
	}

	void editor_camera::tick(f32 dt_seconds)
	{
		if (_is_active == 0)
			return;

		update_rotation();
		apply_movement(dt_seconds);
	}

	void editor_camera::update_rotation()
	{
		_yaw_degrees -= _mouse_delta.x * _mouse_sensitivity;
		_pitch_degrees -= _mouse_delta.y * _mouse_sensitivity;
		_pitch_degrees = math::clamp(_pitch_degrees, -89.0f, 89.0f);

		entity_manager& manager = _world->get_entity_manager();
		const quat		new_rot = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		manager.set_entity_rotation(_entity, new_rot);
		_world->set_tool_camera_rot(new_rot);

		_mouse_delta = vector2::zero;
	}

	void editor_camera::apply_movement(f32 dt_seconds)
	{
		entity_manager& manager = _world->get_entity_manager();
		const quat&		rot		= manager.get_entity_rotation(_entity);

		const vector3 forward  = rot.get_forward();
		const vector3 right	   = rot.get_right();
		const vector3 up	   = vector3::up;
		vector3		  move_dir = (forward * _direction_input.z + right * _direction_input.x + up * _direction_input.y);

		if (move_dir.is_zero())
			return;
		move_dir.normalize();

		const vector3& pos		= manager.get_entity_position(_entity);
		const vector3  position = pos + (move_dir * (_current_move_speed * dt_seconds));
		manager.set_entity_position(_entity, position);

		_world->set_tool_camera_pos(position);
	}
}
