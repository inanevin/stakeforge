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
#include "game_plugin_runtime.hpp"
#include "core/reflection/plugin_type_schema.hpp"

#include "core/io/assert.hpp"
#include "core/io/log.hpp"
#include "core/platform/process.hpp"
#include "core/runtime/world_renderer/stock_world_renderer.hpp"

namespace sfg
{
	game_plugin_runtime_t::game_plugin_runtime_t() = default;

	game_plugin_runtime_t::~game_plugin_runtime_t() = default;

	bool game_plugin_runtime_t::load(const char* path, string_t& diagnostics)
	{
		SFG_ASSERT(_library == nullptr);

		return stage(path, diagnostics) && activate_staged(diagnostics);
	}

	bool game_plugin_runtime_t::stage(const char* path, string_t& diagnostics)
	{
		if (_staged_library != nullptr && !discard_staged(diagnostics))
			return false;

		SFG_ASSERT(_playing_world_count == 0);

		void* const library = process::load_library(path, diagnostics);

		if (library == nullptr)
			return false;

		_staged_library = library;

		const game_plugin_entry_fn entry	  = reinterpret_cast<game_plugin_entry_fn>(process::get_library_symbol(library, "sfg_get_game_plugin_callbacks", diagnostics));
		game_plugin_callbacks_t	   callbacks  = {};
		const bool				   compatible = entry != nullptr && entry(GAME_PLUGIN_API_VERSION, callbacks);
		const bool				   complete = compatible && callbacks.register_types != nullptr && callbacks.load != nullptr && callbacks.unload != nullptr &&
											  (callbacks.begin_play != nullptr && callbacks.end_play != nullptr && callbacks.tick_logic != nullptr && callbacks.tick_physics != nullptr && callbacks.tick_animation != nullptr && callbacks.key_event != nullptr &&
											   callbacks.mouse_button_event != nullptr && callbacks.mouse_move_event != nullptr && callbacks.mouse_wheel_event != nullptr && callbacks.focus_event != nullptr && callbacks.physics_contact != nullptr &&
											   callbacks.animation_event != nullptr && callbacks.canvas_event != nullptr);

		if (!complete || ((callbacks.create_world_renderer == nullptr) != (callbacks.destroy_world_renderer == nullptr)))
		{
			if (entry != nullptr)
				diagnostics = "Game plugin has an incompatible API or missing callbacks: " + string_t(path);

			SFG_ERR("{0}", diagnostics);

			string_t unload_diagnostics = {};

			if (!discard_staged(unload_diagnostics))
				diagnostics += "\n" + unload_diagnostics;

			return false;
		}

		_staged_callbacks = callbacks;
		_staged_path	  = path;
		_staged_schema.reset(new plugin_type_schema_t(reflection_owner_e::game_plugin));
		callbacks.register_types(*_staged_schema);

		if (!_staged_schema->validate(diagnostics))
		{
			string_t unload_diagnostics = {};

			if (!discard_staged(unload_diagnostics))
				diagnostics += "\n" + unload_diagnostics;

			return false;
		}

		return true;
	}

	bool game_plugin_runtime_t::discard_staged(string_t& diagnostics)
	{
		SFG_ASSERT(_staged_library != nullptr);

		if (!process::unload_library(_staged_library, diagnostics))
			return false;

		_staged_library	  = nullptr;
		_staged_callbacks = {};
		_staged_schema.reset();
		_staged_path.resize(0);

		return true;
	}

	bool game_plugin_runtime_t::activate_staged(string_t& diagnostics, game_plugin_activated_fn activated)
	{
		SFG_ASSERT(_staged_library != nullptr);
		SFG_ASSERT(_playing_world_count == 0);

		const plugin_type_schema_t		 empty_schema	 = {};
		const plugin_type_schema_t&		 previous_schema = _schema ? *_schema : empty_schema;
		const plugin_type_schema_delta_t delta			 = previous_schema.compare(*_staged_schema);

		if (_library != nullptr && !deactivate(diagnostics))
			return false;

		unique_t<plugin_type_schema_t> previous = std::move(_schema);
		_schema									= std::move(_staged_schema);
		_library								= std::exchange(_staged_library, nullptr);
		_callbacks								= std::exchange(_staged_callbacks, {});
		_path									= std::move(_staged_path);
		_schema->register_reflection_types();

		if (activated != nullptr)
			activated(previous_schema, *_schema, delta);

		_callbacks.load();

		SFG_INFO("loaded game plugin: {0}. Types: {1} added, {2} removed, {3} migrated.", _path, delta.added.size(), delta.removed.size(), delta.layout_changed.size());

		return true;
	}

	bool game_plugin_runtime_t::deactivate(string_t& diagnostics)
	{
		SFG_ASSERT(_library != nullptr && _playing_world_count == 0 && _world_renderer_count == 0);

		_callbacks.unload();
		reflection_registry_t::get().remove_plugin_types(reflection_owner_e::game_plugin);

		if (!process::unload_library(_library, diagnostics))
		{
			_schema->register_reflection_types();
			_callbacks.load();

			return false;
		}

		_library   = nullptr;
		_callbacks = {};

		return true;
	}

	bool game_plugin_runtime_t::unload(string_t& diagnostics)
	{
		SFG_ASSERT(_schema != nullptr);

		if (_staged_library != nullptr && !discard_staged(diagnostics))
			return false;

		if (_library != nullptr && !deactivate(diagnostics))
			return false;

		SFG_INFO("unloaded game plugin: {0}", _path);
		_schema.reset();
		_path.resize(0);

		return true;
	}

	world_renderer_t* game_plugin_runtime_t::create_world_renderer(world_t& world)
	{
		world_renderer_t* renderer = _callbacks.create_world_renderer != nullptr ? _callbacks.create_world_renderer(world) : new stock_world_renderer_t();

		if (renderer == nullptr)
		{
			SFG_ERR("failed to create the game world renderer");
			return nullptr;
		}

		++_world_renderer_count;

		return renderer;
	}

	void game_plugin_runtime_t::destroy_world_renderer(world_renderer_t* renderer)
	{
		SFG_ASSERT(_world_renderer_count != 0);

		if (_callbacks.destroy_world_renderer != nullptr)
			_callbacks.destroy_world_renderer(renderer);
		else
			delete renderer;

		--_world_renderer_count;
	}

	void game_plugin_runtime_t::begin_play(world_t& world)
	{
		++_playing_world_count;

		if (_callbacks.begin_play != nullptr)
			_callbacks.begin_play(world);
	}

	void game_plugin_runtime_t::end_play(world_t& world)
	{
		SFG_ASSERT(_playing_world_count != 0);

		if (_callbacks.end_play != nullptr)
			_callbacks.end_play(world);

		--_playing_world_count;
	}

	void game_plugin_runtime_t::tick_logic(world_t& world, f32 dt)
	{
		if (_callbacks.tick_logic != nullptr)
			_callbacks.tick_logic(world, dt);
	}

	void game_plugin_runtime_t::tick_physics(world_t& world, f32 dt)
	{
		if (_callbacks.tick_physics != nullptr)
			_callbacks.tick_physics(world, dt);
	}

	void game_plugin_runtime_t::tick_animation(world_t& world, f32 dt)
	{
		if (_callbacks.tick_animation != nullptr)
			_callbacks.tick_animation(world, dt);
	}

	void game_plugin_runtime_t::key_event(world_t& world, input_code key, u16 scan_code, input_action action)
	{
		if (_callbacks.key_event != nullptr)
			_callbacks.key_event(world, key, scan_code, action);
	}

	void game_plugin_runtime_t::mouse_button_event(world_t& world, input_code button, input_action action, f32 position_x, f32 position_y)
	{
		if (_callbacks.mouse_button_event != nullptr)
			_callbacks.mouse_button_event(world, button, action, position_x, position_y);
	}

	void game_plugin_runtime_t::mouse_move_event(world_t& world, f32 position_x, f32 position_y, f32 delta_x, f32 delta_y)
	{
		if (_callbacks.mouse_move_event != nullptr)
			_callbacks.mouse_move_event(world, position_x, position_y, delta_x, delta_y);
	}

	void game_plugin_runtime_t::mouse_wheel_event(world_t& world, f32 position_x, f32 position_y, f32 delta)
	{
		if (_callbacks.mouse_wheel_event != nullptr)
			_callbacks.mouse_wheel_event(world, position_x, position_y, delta);
	}

	void game_plugin_runtime_t::focus_event(world_t& world, bool focused)
	{
		if (_callbacks.focus_event != nullptr)
			_callbacks.focus_event(world, focused);
	}

	void game_plugin_runtime_t::physics_contact(world_t& world, const physics_contact_event_t& contact)
	{
		if (_callbacks.physics_contact != nullptr)
			_callbacks.physics_contact(world, contact);
	}

	void game_plugin_runtime_t::animation_event(world_t& world, entity_id_t entity, sid_t event)
	{
		if (_callbacks.animation_event != nullptr)
			_callbacks.animation_event(world, entity, event);
	}

	void game_plugin_runtime_t::canvas_event(world_t& world, const canvas_event_t& event)
	{
		if (_callbacks.canvas_event != nullptr)
			_callbacks.canvas_event(world, event);
	}
}
