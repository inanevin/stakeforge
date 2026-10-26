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
#include <sfg/core/runtime/scripting/plugin_api.hpp>
#include <sfg/core/runtime/world/world.hpp>

namespace game
{
	using namespace sfg;

	void load()
	{
	}

	void unload()
	{
	}

	void begin_play(world_t& world)
	{
	}

	void end_play(world_t& world)
	{
	}

	void tick_logic(world_t& world, f32 dt)
	{
	}

	void tick_physics(world_t& world, f32 dt)
	{
	}

	void tick_animation(world_t& world, f32 dt)
	{
	}

	void draw_debug(world_t& world)
	{
	}

	void key_event(world_t& world, u16 key, u16 scan_code, u8 action)
	{
	}

	void mouse_button_event(world_t& world, u8 button, u8 action, f32 position_x, f32 position_y)
	{
	}

	void mouse_move_event(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
	}

	void mouse_wheel_event(world_t& world, f32 position_x, f32 position_y, f32 delta)
	{
	}

	void focus_event(world_t& world, bool focused)
	{
	}

	void physics_contact(world_t& world, const physics_contact_event_t& contact)
	{
	}

	void animation_event(world_t& world, entity_id_t entity, sid_t event)
	{
	}

	void canvas_event(world_t& world, const canvas_event_t& event)
	{
	}
}

SFG_PLUGIN_ENTRY bool sfg_get_plugin_callbacks(u32 api_version, sfg::plugin_callbacks_t& callbacks)
{
	if (api_version != sfg::PLUGIN_API_VERSION)
		return false;

	callbacks = {
		.load				= game::load,
		.unload				= game::unload,
		.begin_play			= game::begin_play,
		.end_play			= game::end_play,
		.tick_logic			= game::tick_logic,
		.tick_physics		= game::tick_physics,
		.tick_animation		= game::tick_animation,
		.draw_debug			= game::draw_debug,
		.key_event			= game::key_event,
		.mouse_button_event = game::mouse_button_event,
		.mouse_move_event	= game::mouse_move_event,
		.mouse_wheel_event	= game::mouse_wheel_event,
		.focus_event		= game::focus_event,
		.physics_contact	= game::physics_contact,
		.animation_event	= game::animation_event,
		.canvas_event		= game::canvas_event,
	};

	return true;
}
