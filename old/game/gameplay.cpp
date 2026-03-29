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

#include "gameplay.hpp"
#include "app/app.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/components/comp_canvas.hpp"
#include "platform/window_common.hpp"
#include "platform/window.hpp"
#include "input/input_mappings.hpp"
#include "resources/entity_template.hpp"
#include "world/entity_manager.hpp"
#include "math/quat.hpp"
#include "math/math.hpp"

#include "gui/vekt.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	vekt::id	   rect;
	vekt::builder* b;

	void gameplay::on_world_begin(world& w)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		_player_entity = em.find_entity("Player");
		if (!_player_entity.is_null())
			_camera_entity = em.find_entity(_player_entity, "PlayerCamera");
		if (_camera_entity.is_null())
			_camera_entity = em.find_entity("PlayerCamera");

		if (!_camera_entity.is_null())
		{
			_camera_comp = em.get_entity_component<comp_camera>(_camera_entity);
			if (!_camera_comp.is_null())
			{
				comp_camera& comp = cm.get_component<comp_camera>(_camera_comp);
				comp.set_main(w);
			}
		}

		if (!_player_entity.is_null())
			_player_controller = em.get_entity_component<comp_character_controller>(_player_entity);

		//_bullet_template = rm.get_resource_handle_by_hash<entity_template>(TO_SID("assets/entity_templates/bullet.stkent"));

		_direction_input	= vector3::zero;
		_mouse_delta		= vector2::zero;
		_yaw_degrees		= 0.0f;
		_pitch_degrees		= 0.0f;
		_current_move_speed = _base_move_speed;
		_is_active			= !_player_entity.is_null() && !_camera_entity.is_null() && !_player_controller.is_null();

		_app.get_main_window().confine_cursor(cursor_confinement::pointer);
		_app.get_main_window().set_cursor_visible(false);
		_is_looking = true;

		const world_handle cnv_entity = em.create_entity("cnv");
		const world_handle cnv_comp	  = cm.add_component<comp_canvas>(cnv_entity);
		comp_canvas&	   comp		  = cm.get_component<comp_canvas>(cnv_comp);
		comp.update_counts_and_init(w, 512, 4);
		b = comp.get_builder();

		rect = b->allocate();
		{
			vekt::pos_props& pp = b->widget_get_pos_props(rect);
			pp.pos.x			= 0.5f;
			pp.pos.y			= 0.5f;
			pp.flags			= vekt::pos_flags::pf_x_abs | vekt::pos_flags::pf_y_abs | vekt::pos_flags::pf_x_anchor_center | vekt::pos_flags::pf_y_anchor_center;

			vekt::size_props& sz = b->widget_get_size_props(rect);
			sz.size.x			 = 30;
			sz.size.y			 = 30;
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;

			vekt::widget_gfx& gfx = b->widget_get_gfx(rect);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect;
			gfx.color			  = vector4(1, 0, 0, 1);
		}
		b->widget_add_child(b->get_root(), rect);
		b->build_hierarchy();
	}

	void gameplay::on_world_end(world& w)
	{
		_player_entity	   = {};
		_player_controller = {};
		_camera_entity	   = {};
		_camera_comp	   = {};
		//_bullet_template = {};
		_direction_input = vector3::zero;
		_mouse_delta	 = vector2::zero;
		_is_looking		 = false;
		_is_active		 = 0;
		_app.get_main_window().confine_cursor(cursor_confinement::none);
		_app.get_main_window().set_cursor_visible(true);
	}

	namespace
	{
		bool project_point(const matrix4x4& view_proj, const vector2& panel_pos, const vector2& panel_size, const vector3& world_pos, vector2& out)
		{
			vector4 clip = view_proj * vector4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
			if (math::abs(clip.w) < MATH_EPS)
				return false;

			if (clip.w < 0.0f)
				return false;

			const f32 inv_w = 1.0f / clip.w;
			const f32 ndc_x = clip.x * inv_w;
			const f32 ndc_y = clip.y * inv_w;

			out.x = panel_pos.x + (ndc_x * 0.5f + 0.5f) * panel_size.x;
			out.y = panel_pos.y + (1.0f - (ndc_y * 0.5f + 0.5f)) * panel_size.y;
			return true;
		}
	}

	void gameplay::on_world_tick(world& w, f32 dt, const vector2ui16& game_res)
	{
		if (_is_active == 0 || _player_entity.is_null() || _camera_entity.is_null() || _player_controller.is_null())
			return;

		_yaw_degrees -= _mouse_delta.x * _mouse_sensitivity;
		_pitch_degrees -= _mouse_delta.y * _mouse_sensitivity;
		_pitch_degrees = math::clamp(_pitch_degrees, -89.0f, 89.0f);

		entity_manager&	   em	   = w.get_entity_manager();
		component_manager& cm	   = w.get_comp_manager();
		const quat		   new_rot = quat::from_euler(_pitch_degrees, _yaw_degrees, 0.0f);
		em.set_entity_rotation(_camera_entity, new_rot);

		_mouse_delta = vector2::zero;

		const quat&	  rot	   = em.get_entity_rotation_abs(_camera_entity);
		const vector3 forward  = rot.get_forward();
		const vector3 right	   = rot.get_right();
		const vector3 up	   = vector3::up;
		vector3		  move_dir = (forward * _direction_input.z + right * _direction_input.x + up * _direction_input.y);

		if (!move_dir.is_zero())
			move_dir.normalize();

		comp_character_controller& controller = cm.get_component<comp_character_controller>(_player_controller);
		controller.set_target_velocity(move_dir);

		{
			const world_handle main_cam_entity = em.get_main_camera_entity();
			const world_handle main_cam_comp   = em.get_main_camera_comp();
			if (main_cam_entity.is_null() || main_cam_comp.is_null())
				return;

			component_manager& cm		   = w.get_comp_manager();
			comp_camera&	   camera_comp = cm.get_component<comp_camera>(main_cam_comp);

			const f32		aspect	  = (f32)game_res.x / (f32)game_res.y;
			const matrix4x4 view	  = matrix4x4::view(em.get_entity_rotation_abs(main_cam_entity), em.get_entity_position_abs(main_cam_entity));
			const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(camera_comp.get_fov_degrees(), aspect, camera_comp.get_near(), camera_comp.get_far());
			matrix4x4		view_proj = proj * view;
			vector2			out		  = vector2::zero;
			project_point(view_proj, vector2::zero, game_res, vector3::zero, out);
			b->widget_get_pos_props(rect).pos = out;
		}
	}

	void gameplay::on_window_event(const window_event& ev, window* wnd)
	{
		if (_is_active == 0)
			return;

		const u16 button = ev.button;

		switch (ev.type)
		{
		case window_event_type::focus: {
			//_direction_input	= vector3::zero;
			//_mouse_delta		= vector2::zero;
			//_is_looking			= false;
			//_current_move_speed = _base_move_speed;
			//_app.get_main_window().confine_cursor(cursor_confinement::none);
			//_app.get_main_window().set_cursor_visible(true);
			return;
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
			break;
		}
		case window_event_type::mouse: {
			if (button == static_cast<u16>(input_code::mouse_0) && ev.sub_type == window_event_sub_type::press)
			{
				world&			  w	 = _app.get_world();
				entity_manager&	  em = w.get_entity_manager();
				resource_manager& rm = w.get_resource_manager();
				if (rm.is_valid<entity_template>(_bullet_template))
				{
					const world_handle bullet = em.instantiate_template(_bullet_template);
					if (!bullet.is_null())
					{
						const quat&	  cam_rot	= em.get_entity_rotation_abs(_camera_entity);
						const vector3 cam_pos	= em.get_entity_position_abs(_camera_entity);
						const vector3 spawn_pos = cam_pos + (cam_rot.get_forward() * 2.0f);
						em.set_entity_position_abs(bullet, spawn_pos);
					}
				}
			}
			break;
		}

		case window_event_type::delta: {
			if (_is_looking)
			{
				_mouse_delta.x += static_cast<f32>(ev.value.x);
				_mouse_delta.y += static_cast<f32>(ev.value.y);
			}
			else
			{
				_mouse_delta = vector2::zero;
			}
			break;
		}
		default:
			break;
		}
	}
}
