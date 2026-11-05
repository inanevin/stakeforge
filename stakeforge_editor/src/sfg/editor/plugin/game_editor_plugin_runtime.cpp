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
#include "game_editor_plugin_runtime.hpp"
#include <sfg/core/reflection/plugin_type_schema.hpp>

#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/process.hpp>

namespace sfg
{
	game_editor_plugin_runtime_t::game_editor_plugin_runtime_t() = default;

	game_editor_plugin_runtime_t::~game_editor_plugin_runtime_t() = default;

	bool game_editor_plugin_runtime_t::load(const char* path, string_t& diagnostics)
	{
		SFG_ASSERT(_library == nullptr);

		return stage(path, diagnostics) && activate_staged(diagnostics);
	}

	bool game_editor_plugin_runtime_t::stage(const char* path, string_t& diagnostics)
	{
		if (_staged_library != nullptr && !discard_staged(diagnostics))
			return false;

		void* const library = process::load_library(path, diagnostics);

		if (library == nullptr)
			return false;

		_staged_library = library;

		const game_editor_plugin_entry_fn entry		 = reinterpret_cast<game_editor_plugin_entry_fn>(process::get_library_symbol(library, "sfg_get_game_editor_plugin_callbacks", diagnostics));
		game_editor_plugin_callbacks_t	  callbacks	 = {};
		const bool						  compatible = entry != nullptr && entry(GAME_EDITOR_PLUGIN_API_VERSION, callbacks);
		const bool						  complete	 = compatible && callbacks.register_types != nullptr && callbacks.load != nullptr && callbacks.unload != nullptr;

		if (!complete)
		{
			if (entry != nullptr)
				diagnostics = "Game editor plugin has an incompatible API or missing callbacks: " + string_t(path);

			SFG_ERR("{0}", diagnostics);

			string_t unload_diagnostics = {};

			if (!discard_staged(unload_diagnostics))
				diagnostics += "\n" + unload_diagnostics;

			return false;
		}

		_staged_callbacks = callbacks;
		_staged_path	  = path;
		_staged_schema.reset(new plugin_type_schema_t(reflection_owner_e::game_editor_plugin, reflection_owner_e::game_plugin));
		callbacks.register_types(*_staged_schema);

		bool valid = _staged_schema->validate(diagnostics);

		for (const reflected_type_descriptor_t& type : _staged_schema->get_types())
		{
			if (!type.flags.is_set(reflected_type_flag_component | reflected_type_flag_system_component | reflected_type_flag_tag_component))
				continue;

			diagnostics = "Game components must be registered by the game plugin.";
			SFG_ERR("{0}", diagnostics);
			valid = false;
			break;
		}

		if (!valid)
		{
			string_t unload_diagnostics = {};

			if (!discard_staged(unload_diagnostics))
				diagnostics += "\n" + unload_diagnostics;

			return false;
		}

		return true;
	}

	bool game_editor_plugin_runtime_t::discard_staged(string_t& diagnostics)
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

	bool game_editor_plugin_runtime_t::activate_staged(string_t& diagnostics)
	{
		SFG_ASSERT(_staged_library != nullptr);

		if (_library != nullptr && !deactivate(diagnostics))
			return false;

		_schema	   = std::move(_staged_schema);
		_library   = std::exchange(_staged_library, nullptr);
		_callbacks = std::exchange(_staged_callbacks, {});
		_path	   = std::move(_staged_path);
		_schema->register_reflection_types();

		_callbacks.load();

		SFG_INFO("loaded game editor plugin: {0}", _path);

		return true;
	}

	bool game_editor_plugin_runtime_t::deactivate(string_t& diagnostics)
	{
		SFG_ASSERT(_library != nullptr);

		_callbacks.unload();
		reflection_registry_t::get().remove_plugin_types(reflection_owner_e::game_editor_plugin);

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

	bool game_editor_plugin_runtime_t::unload(string_t& diagnostics)
	{
		SFG_ASSERT(_schema != nullptr);

		if (_staged_library != nullptr && !discard_staged(diagnostics))
			return false;

		if (_library != nullptr && !deactivate(diagnostics))
			return false;

		SFG_INFO("unloaded game editor plugin: {0}", _path);
		_schema.reset();
		_path.resize(0);

		return true;
	}

	void game_editor_plugin_runtime_t::tick_logic(world_t& world, f32 dt)
	{
		if (_callbacks.tick_logic != nullptr)
			_callbacks.tick_logic(world, dt);
	}
}
