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

#pragma once

#include "world/editor_world_camera.hpp"

namespace sfg
{
	class editor_world_camera_fly_t final : public editor_world_camera_t
	{
	public:
		editor_world_camera_fly_t()												= default;
		~editor_world_camera_fly_t() override									= default;
		editor_world_camera_fly_t(const editor_world_camera_fly_t&)				= delete;
		editor_world_camera_fly_t& operator=(const editor_world_camera_fly_t&)	= delete;
		editor_world_camera_fly_t(editor_world_camera_fly_t&& other)			= delete;
		editor_world_camera_fly_t& operator=(editor_world_camera_fly_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world) override;
		void uninit(world_t& world) override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void pass_input(world_t& world, const editor_world_camera_input_t& input) override;
		void tick(world_t& world, f32 dt_seconds) override;
		void serialize(const world_t& world, nlohmann::json& out_json) const override;
		void deserialize(world_t& world, const nlohmann::json& in_json) override;
		void fit_to_bounds(world_t& world, const aabb_t& bounds) override;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline entity_id_t get_entity() const override
		{
			return _camera_entity;
		}

	private:
		vec3f_t		_direction_input	  = vec3f_t::zero;
		vec2f_t		_mouse_delta		  = vec2f_t::zero;
		entity_id_t _camera_entity		  = NULL_ENTITY_ID;
		f32			_camera_yaw_degrees	  = 0.0f;
		f32			_camera_pitch_degrees = 0.0f;
		f32			_current_move_speed	  = 12.0f;
	};
}
