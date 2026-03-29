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

#include "world/common_world.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "world/world_constants.hpp"
#include <array>

namespace SFG
{
	struct window_event;
	class world;
	class window;
	class game;

	class editor_camera
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(world& world, window& wnd);
		void uninit();
		bool on_window_event(const window_event& ev);
		void tick(f32 dt_seconds);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void activate();
		void deactivate();
		void set_is_looking(bool b);

		// -----------------------------------------------------------------------------
		// accessors/muts
		// -----------------------------------------------------------------------------

		inline void set_game(game* g)
		{
			_game = g;
		}

		inline world_handle get_entity() const
		{
			return _entity;
		}

		inline bool get_is_looking() const
		{
			return _is_looking;
		}

	private:
		void reset_runtime();
		void update_rotation();
		void apply_movement(f32 dt_seconds);

	private:
		window* _window = nullptr;
		world*	_world	= nullptr;
		game*	_game	= nullptr;

		world_handle _entity	   = {};
		world_handle _camera_trait = {};

		vector3 _direction_input	= vector3::zero;
		vector2 _mouse_delta		= vector2::zero;
		f32		_yaw_degrees		= 0.0f;
		f32		_pitch_degrees		= 0.0f;
		f32		_current_move_speed = 12.0f;
		f32		_base_move_speed	= 12.0f;
		f32		_boost_multiplier	= 8.0f;
		f32		_mouse_sensitivity	= 0.08f;
		bool	_is_looking			= false;
		bool	_is_active			= false;
	};
}
