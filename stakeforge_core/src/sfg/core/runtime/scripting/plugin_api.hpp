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

#include "core/common/size_definitions.hpp"
#include "core/runtime/world/ecs_defs.hpp"

#define SFG_PLUGIN_ENTRY extern "C" __declspec(dllexport)

namespace sfg
{
	class world_t;
	struct physics_contact_event_t;
	struct canvas_event_t;

	inline constexpr u32 PLUGIN_API_VERSION = 1;

	struct plugin_callbacks_t
	{
		void (*load)()																					   = nullptr;
		void (*unload)()																				   = nullptr;
		void (*begin_play)(world_t& world)																   = nullptr;
		void (*end_play)(world_t& world)																   = nullptr;
		void (*tick_logic)(world_t& world, f32 dt)														   = nullptr;
		void (*tick_physics)(world_t& world, f32 dt)													   = nullptr;
		void (*tick_animation)(world_t& world, f32 dt)													   = nullptr;
		void (*draw_debug)(world_t& world)																   = nullptr;
		void (*key_event)(world_t& world, u16 key, u16 scan_code, u8 action)							   = nullptr;
		void (*mouse_button_event)(world_t& world, u8 button, u8 action, f32 position_x, f32 position_y)   = nullptr;
		void (*mouse_move_event)(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y) = nullptr;
		void (*mouse_wheel_event)(world_t& world, f32 position_x, f32 position_y, f32 delta)			   = nullptr;
		void (*focus_event)(world_t& world, bool focused)												   = nullptr;
		void (*physics_contact)(world_t& world, const physics_contact_event_t& contact)					   = nullptr;
		void (*animation_event)(world_t& world, entity_id_t entity, sid_t event)						   = nullptr;
		void (*canvas_event)(world_t& world, const canvas_event_t& event)								   = nullptr;
	};

	using plugin_entry_fn = bool (*)(u32 api_version, plugin_callbacks_t& callbacks);
}
