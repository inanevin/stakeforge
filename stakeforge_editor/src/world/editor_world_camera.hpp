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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct aabb_t;
	class world_t;

	enum class editor_world_camera_type_e : u8
	{
		fly,
		orbit,
	};

	struct editor_world_camera_input_t
	{
		vec3f_t direction_delta = vec3f_t::zero;
		vec2f_t mouse_delta		= vec2f_t::zero;
		f32		move_speed		= 0.0f;
		f32		wheel_delta		= 0.0f;
		bool	set_move_speed	= false;
		bool	reset			= false;
	};

	class editor_world_camera_t
	{
	public:
		editor_world_camera_t()											= default;
		virtual ~editor_world_camera_t()								= default;
		editor_world_camera_t(const editor_world_camera_t&)				= delete;
		editor_world_camera_t& operator=(const editor_world_camera_t&)	= delete;
		editor_world_camera_t(editor_world_camera_t&& other)			= delete;
		editor_world_camera_t& operator=(editor_world_camera_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		virtual void init(world_t& world)	= 0;
		virtual void uninit(world_t& world) = 0;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		virtual void pass_input(world_t& world, const editor_world_camera_input_t& input) = 0;
		virtual void tick(world_t& world, f32 dt_seconds)								  = 0;
		virtual void serialize(const world_t& world, nlohmann::json& out_json) const	  = 0;
		virtual void deserialize(world_t& world, const nlohmann::json& in_json)			  = 0;
		virtual void fit_to_bounds(world_t& world, const aabb_t& bounds)
		{
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		virtual entity_id_t get_entity() const = 0;

		inline bool is_focus_enabled() const
		{
			return _focus_enabled;
		}

	protected:
		static void	   calculate_focus_target(const vec3f_t& camera_position, const quat_t& camera_rotation, const aabb_t& bounds, vec3f_t& target_position, quat_t& target_rotation);
		static vec2f_t get_camera_angles(const quat_t& rotation);
		void		   begin_focus(world_t& world, const vec3f_t& target_position, const quat_t& target_rotation);
		void		   cancel_focus();
		bool		   tick_focus(world_t& world, f32 dt_seconds);

	private:
		quat_t	_focus_start_rotation  = quat_t::identity;
		quat_t	_focus_target_rotation = quat_t::identity;
		vec3f_t _focus_start_position  = vec3f_t::zero;
		vec3f_t _focus_target_position = vec3f_t::zero;
		f32		_focus_elapsed		   = 0.0f;
		f32		_focus_duration		   = 0.0f;
		bool	_focus_enabled		   = false;
	};
}
