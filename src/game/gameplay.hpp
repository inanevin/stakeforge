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

#pragma once

#include "common/string_id.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "resources/common_resources.hpp"
#include "world/world_constants.hpp"

namespace SFG
{
	class world;
	class app;
	class window;
	struct window_event;
	struct vector2ui16;

	class gameplay
	{
	public:
		gameplay(app& app) : _app(app){};

		void on_world_begin(world& w);
		void on_world_end(world& w);
		void on_world_tick(world& w, f32 dt, const vector2ui16& game_res);
		void on_window_event(const window_event& ev, window* wnd);

	private:
		app&			_app;
		window*			_window				= nullptr;
		world_handle	_player_entity		= {};
		world_handle	_player_controller	= {};
		world_handle	_camera_entity		= {};
		world_handle	_camera_comp		= {};
		resource_handle _bullet_template	= {};
		vector3			_direction_input	= vector3::zero;
		vector2			_mouse_delta		= vector2::zero;
		f32				_yaw_degrees		= 0.0f;
		f32				_pitch_degrees		= 0.0f;
		f32				_current_move_speed = 12.0f;
		f32				_base_move_speed	= 12.0f;
		f32				_boost_multiplier	= 8.0f;
		f32				_mouse_sensitivity	= 0.08f;
		bool			_is_looking			= false;
		u8				_is_active			= 0;
	};

}
