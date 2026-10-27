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

#include "core/common/sfg_exports.hpp"
#include "core/data/string.hpp"
#include "core/data/unique.hpp"
#include "core/runtime/scripting/plugin_api.hpp"

namespace sfg
{
	struct native_component_schema_delta_t;
	using native_plugin_activated_fn = void (*)(const native_component_schema_t& previous, const native_component_schema_t& current, const native_component_schema_delta_t& delta);

	class SFG_API plugin_runtime_t final
	{
	public:
		plugin_runtime_t();
		~plugin_runtime_t();
		plugin_runtime_t(const plugin_runtime_t& other)			   = delete;
		plugin_runtime_t& operator=(const plugin_runtime_t& other) = delete;

		static plugin_runtime_t& get()
		{
			static plugin_runtime_t instance = {};

			return instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool load(const char* path, string_t& diagnostics);
		bool unload(string_t& diagnostics);
		bool stage(const char* path, string_t& diagnostics);
		bool activate_staged(string_t& diagnostics, native_plugin_activated_fn activated = nullptr);
		bool discard_staged(string_t& diagnostics);
		void begin_play(world_t& world);
		void end_play(world_t& world);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick_logic(world_t& world, f32 dt);
		void tick_physics(world_t& world, f32 dt);
		void tick_animation(world_t& world, f32 dt);
		void draw_debug(world_t& world);
		void key_event(world_t& world, input_code key, u16 scan_code, input_action action);
		void mouse_button_event(world_t& world, input_code button, input_action action, f32 position_x, f32 position_y);
		void mouse_move_event(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y);
		void mouse_wheel_event(world_t& world, f32 position_x, f32 position_y, f32 delta);
		void focus_event(world_t& world, bool focused);
		void physics_contact(world_t& world, const physics_contact_event_t& contact);
		void animation_event(world_t& world, entity_id_t entity, sid_t event);
		void canvas_event(world_t& world, const canvas_event_t& event);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool is_loaded() const
		{
			return _library != nullptr;
		}

		bool is_staged() const
		{
			return _staged_library != nullptr;
		}

		const string_t& get_path() const
		{
			return _path;
		}

	private:
		unique_t<native_component_schema_t> _schema			  = {};
		unique_t<native_component_schema_t> _staged_schema	  = {};
		plugin_callbacks_t					_staged_callbacks = {};
		string_t							_staged_path	  = {};
		void*								_staged_library	  = nullptr;
		plugin_callbacks_t					_callbacks		  = {};
		string_t							_path;
		void*								_library			 = nullptr;
		u32									_playing_world_count = 0;
	};
}
