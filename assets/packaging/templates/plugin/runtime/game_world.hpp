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

#include <sfg/core/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;
	enum input_code : unsigned short;
	enum class input_action;
	struct physics_contact_event_t;
	struct canvas_event_t;
}

namespace game
{
	class game_world_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		// set up gameplay when the world starts playing.
		static void begin_play(sfg::world_t& world);

		// clean up gameplay when the world stops playing.
		static void end_play(sfg::world_t& world);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		// update gameplay during the logic tick.
		static void tick_logic(sfg::world_t& world, f32 dt);

		// update gameplay during the physics tick.
		static void tick_physics(sfg::world_t& world, f32 dt);

		// update gameplay during the animation tick.
		static void tick_animation(sfg::world_t& world, f32 dt);

		// -----------------------------------------------------------------------------
		// input
		// -----------------------------------------------------------------------------

		// handle keyboard input for the world.
		static void key_event(sfg::world_t& world, sfg::input_code key, u16 scan_code, sfg::input_action action);

		// handle mouse button input and its cursor position.
		static void mouse_button_event(sfg::world_t& world, sfg::input_code button, sfg::input_action action, f32 position_x, f32 position_y);

		// handle cursor movement and its delta.
		static void mouse_move_event(sfg::world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y);

		// handle mouse wheel input at the cursor position.
		static void mouse_wheel_event(sfg::world_t& world, f32 position_x, f32 position_y, f32 delta);

		// handle changes to the world input focus.
		static void focus_event(sfg::world_t& world, bool focused);

		// -----------------------------------------------------------------------------
		// events
		// -----------------------------------------------------------------------------

		// handle a physics contact event.
		static void physics_contact(sfg::world_t& world, const sfg::physics_contact_event_t& contact);

		// handle an animation event from an entity.
		static void animation_event(sfg::world_t& world, sfg::entity_id_t entity, sfg::sid_t event);

		// handle an event from a world canvas.
		static void canvas_event(sfg::world_t& world, const sfg::canvas_event_t& event);
	};
}
