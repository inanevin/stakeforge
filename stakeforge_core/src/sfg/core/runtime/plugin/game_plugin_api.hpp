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

#define SFG_GAME_PLUGIN_ENTRY extern "C" __declspec(dllexport)

namespace sfg
{
	class world_t;
	class world_renderer_t;
	class plugin_type_schema_t;
	enum input_code : unsigned short;
	enum class input_action;
	struct physics_contact_event_t;
	struct canvas_event_t;
	struct vec2u16_t;
	struct world_config_t;
	struct shader_custom_cook_request_t;
	struct shader_custom_cook_result_t;
	enum class world_config_type_e : u8;

	inline constexpr u32 GAME_PLUGIN_API_VERSION = 22;

	struct game_plugin_callbacks_t
	{
		bool (*cook_custom_shader)(const shader_custom_cook_request_t& request, shader_custom_cook_result_t& result)	   = nullptr;
		bool (*get_world_config)(world_config_type_e type, vec2u16_t render_resolution, world_config_t& config)			   = nullptr;
		world_renderer_t* (*create_world_renderer)(world_t& world)														   = nullptr;
		void (*destroy_world_renderer)(world_renderer_t* renderer)														   = nullptr;
		void (*register_types)(plugin_type_schema_t& schema)															   = nullptr;
		void (*load)()																									   = nullptr;
		void (*unload)()																								   = nullptr;
		void (*editor_tick)(world_t& world, f32 dt)																		   = nullptr;
		void (*begin_play)(world_t& world)																				   = nullptr;
		void (*end_play)(world_t& world)																				   = nullptr;
		void (*tick_logic)(world_t& world, f32 dt)																		   = nullptr;
		void (*tick_physics)(world_t& world, f32 dt)																	   = nullptr;
		void (*tick_animation)(world_t& world, f32 dt)																	   = nullptr;
		void (*key_event)(world_t& world, input_code key, u16 scan_code, input_action action)							   = nullptr;
		void (*mouse_button_event)(world_t& world, input_code button, input_action action, f32 position_x, f32 position_y) = nullptr;
		void (*mouse_move_event)(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)				   = nullptr;
		void (*mouse_wheel_event)(world_t& world, f32 position_x, f32 position_y, f32 delta)							   = nullptr;
		void (*focus_event)(world_t& world, bool focused)																   = nullptr;
		void (*physics_contact)(world_t& world, const physics_contact_event_t& contact)									   = nullptr;
		void (*animation_event)(world_t& world, entity_id_t entity, sid_t event)										   = nullptr;
		void (*canvas_event)(world_t& world, const canvas_event_t& event)												   = nullptr;
	};

	using game_plugin_entry_fn = bool (*)(u32 api_version, game_plugin_callbacks_t& callbacks);
}
