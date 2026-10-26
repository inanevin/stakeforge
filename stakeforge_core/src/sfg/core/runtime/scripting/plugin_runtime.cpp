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
#include "core/runtime/scripting/plugin_runtime.hpp"

#include "core/io/assert.hpp"
#include "core/io/log.hpp"
#include "core/platform/process.hpp"

namespace sfg
{
	bool plugin_runtime_t::load(const char* path, string_t& diagnostics)
	{
		SFG_ASSERT(_library == nullptr);
		SFG_ASSERT(_playing_world_count == 0);

		void* const library = process::load_library(path, diagnostics);

		if (library == nullptr)
			return false;

		const plugin_entry_fn entry		 = reinterpret_cast<plugin_entry_fn>(process::get_library_symbol(library, "sfg_get_plugin_callbacks", diagnostics));
		plugin_callbacks_t	  callbacks	 = {};
		const bool			  compatible = entry != nullptr && entry(PLUGIN_API_VERSION, callbacks);
		const bool			  complete	 = compatible && callbacks.load != nullptr && callbacks.unload != nullptr && callbacks.begin_play != nullptr && callbacks.end_play != nullptr && callbacks.tick_logic != nullptr && callbacks.tick_physics != nullptr &&
										   callbacks.tick_animation != nullptr && callbacks.draw_debug != nullptr && callbacks.key_event != nullptr && callbacks.mouse_button_event != nullptr && callbacks.mouse_move_event != nullptr &&
										   callbacks.mouse_wheel_event != nullptr && callbacks.focus_event != nullptr && callbacks.physics_contact != nullptr && callbacks.animation_event != nullptr && callbacks.canvas_event != nullptr;

		if (!complete)
		{
			if (entry != nullptr)
				diagnostics = "Game plugin has an incompatible API or missing callbacks: " + string_t(path);

			SFG_ERR("{0}", diagnostics);

			string_t unload_diagnostics = {};

			if (!process::unload_library(library, unload_diagnostics))
				diagnostics += "\n" + unload_diagnostics;

			return false;
		}

		_library   = library;
		_path	   = path;
		_callbacks = callbacks;
		_callbacks.load();

		SFG_INFO("loaded native game plugin: {0}", _path);
		return true;
	}

	bool plugin_runtime_t::unload(string_t& diagnostics)
	{
		SFG_ASSERT(_library != nullptr);
		SFG_ASSERT(_playing_world_count == 0);

		if (_callbacks.unload != nullptr)
		{
			_callbacks.unload();
			_callbacks = {};
		}

		if (!process::unload_library(_library, diagnostics))
			return false;

		SFG_INFO("unloaded native game plugin: {0}", _path);
		_library = nullptr;
		_path.resize(0);
		return true;
	}

	void plugin_runtime_t::begin_play(world_t& world)
	{
		++_playing_world_count;

		if (_callbacks.begin_play != nullptr)
			_callbacks.begin_play(world);
	}

	void plugin_runtime_t::end_play(world_t& world)
	{
		SFG_ASSERT(_playing_world_count != 0);

		if (_callbacks.end_play != nullptr)
			_callbacks.end_play(world);

		--_playing_world_count;
	}

	void plugin_runtime_t::tick_logic(world_t& world, f32 dt)
	{
		if (_callbacks.tick_logic != nullptr)
			_callbacks.tick_logic(world, dt);
	}

	void plugin_runtime_t::tick_physics(world_t& world, f32 dt)
	{
		if (_callbacks.tick_physics != nullptr)
			_callbacks.tick_physics(world, dt);
	}

	void plugin_runtime_t::tick_animation(world_t& world, f32 dt)
	{
		if (_callbacks.tick_animation != nullptr)
			_callbacks.tick_animation(world, dt);
	}

	void plugin_runtime_t::draw_debug(world_t& world)
	{
		if (_callbacks.draw_debug != nullptr)
			_callbacks.draw_debug(world);
	}

	void plugin_runtime_t::key_event(world_t& world, u16 key, u16 scan_code, u8 action)
	{
		if (_callbacks.key_event != nullptr)
			_callbacks.key_event(world, key, scan_code, action);
	}

	void plugin_runtime_t::mouse_button_event(world_t& world, u8 button, u8 action, f32 position_x, f32 position_y)
	{
		if (_callbacks.mouse_button_event != nullptr)
			_callbacks.mouse_button_event(world, button, action, position_x, position_y);
	}

	void plugin_runtime_t::mouse_move_event(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
		if (_callbacks.mouse_move_event != nullptr)
			_callbacks.mouse_move_event(world, position_x, position_y, delta_x, delta_y);
	}

	void plugin_runtime_t::mouse_wheel_event(world_t& world, f32 position_x, f32 position_y, f32 delta)
	{
		if (_callbacks.mouse_wheel_event != nullptr)
			_callbacks.mouse_wheel_event(world, position_x, position_y, delta);
	}

	void plugin_runtime_t::focus_event(world_t& world, bool focused)
	{
		if (_callbacks.focus_event != nullptr)
			_callbacks.focus_event(world, focused);
	}

	void plugin_runtime_t::physics_contact(world_t& world, const physics_contact_event_t& contact)
	{
		if (_callbacks.physics_contact != nullptr)
			_callbacks.physics_contact(world, contact);
	}

	void plugin_runtime_t::animation_event(world_t& world, entity_id_t entity, sid_t event)
	{
		if (_callbacks.animation_event != nullptr)
			_callbacks.animation_event(world, entity, event);
	}

	void plugin_runtime_t::canvas_event(world_t& world, const canvas_event_t& event)
	{
		if (_callbacks.canvas_event != nullptr)
			_callbacks.canvas_event(world, event);
	}
}
