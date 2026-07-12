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

#include "world/editor_world_camera.hpp"

namespace sfg
{
	class editor_world_camera_orbit_t final : public editor_world_camera_t
	{
	public:
		editor_world_camera_orbit_t()												= default;
		~editor_world_camera_orbit_t() override										= default;
		editor_world_camera_orbit_t(const editor_world_camera_orbit_t&)				= delete;
		editor_world_camera_orbit_t& operator=(const editor_world_camera_orbit_t&)	= delete;
		editor_world_camera_orbit_t(editor_world_camera_orbit_t&& other)			= delete;
		editor_world_camera_orbit_t& operator=(editor_world_camera_orbit_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world) override;
		void uninit(world_t& world) override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void pass_input(const editor_world_camera_input_t& input) override;
		void tick(world_t& world, f32 dt_seconds) override;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline entity_id_t get_entity() const override
		{
			return _camera_entity;
		}

	private:
		vec3f_t		_direction_input	  = vec3f_t::zero;
		vec3f_t		_target				  = vec3f_t::zero;
		vec2f_t		_mouse_delta		  = vec2f_t::zero;
		entity_id_t _camera_entity		  = NULL_ENTITY_ID;
		f32			_camera_yaw_degrees	  = 0.0f;
		f32			_camera_pitch_degrees = 25.0f;
		f32			_distance			  = 8.0f;
		f32			_current_move_speed	  = 12.0f;
	};
}
