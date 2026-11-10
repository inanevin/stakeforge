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

#include "game_world.hpp"

#include <sfg/core/input/input_common.hpp>
#include <sfg/core/input/input_mappings.hpp>
#include <sfg/core/runtime/world/world.hpp>
#include <sfg/core/runtime/world/world_config.hpp>

namespace game
{
	bool game_world_t::get_world_config(sfg::world_config_type_e type, sfg::vec2u16_t render_resolution, sfg::world_config_t& config)
	{
		return true;
	}

	void game_world_t::begin_play(sfg::world_t& world)
	{
	}

	void game_world_t::end_play(sfg::world_t& world)
	{
	}

	void game_world_t::tick_logic(sfg::world_t& world, f32 dt)
	{
	}

	void game_world_t::tick_physics(sfg::world_t& world, f32 dt)
	{
	}

	void game_world_t::tick_animation(sfg::world_t& world, f32 dt)
	{
	}

	void game_world_t::key_event(sfg::world_t& world, sfg::input_code key, u16 scan_code, sfg::input_action action)
	{
	}

	void game_world_t::mouse_button_event(sfg::world_t& world, sfg::input_code button, sfg::input_action action, f32 position_x, f32 position_y)
	{
	}

	void game_world_t::mouse_move_event(sfg::world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
	}

	void game_world_t::mouse_wheel_event(sfg::world_t& world, f32 position_x, f32 position_y, f32 delta)
	{
	}

	void game_world_t::focus_event(sfg::world_t& world, bool focused)
	{
	}

	void game_world_t::physics_contact(sfg::world_t& world, const sfg::physics_contact_event_t& contact)
	{
	}

	void game_world_t::animation_event(sfg::world_t& world, sfg::entity_id_t entity, sfg::sid_t event)
	{
	}

	void game_world_t::canvas_event(sfg::world_t& world, const sfg::canvas_event_t& event)
	{
	}
}
