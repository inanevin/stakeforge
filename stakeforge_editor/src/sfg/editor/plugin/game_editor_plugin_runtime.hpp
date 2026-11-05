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

#include "game_editor_plugin_api.hpp"
#include "editor/editor_exports.hpp"

#include <sfg/core/data/string.hpp>
#include <sfg/core/data/unique.hpp>

namespace sfg
{
	class SFG_EDITOR_API game_editor_plugin_runtime_t final
	{
	public:
		game_editor_plugin_runtime_t();
		~game_editor_plugin_runtime_t();
		game_editor_plugin_runtime_t(const game_editor_plugin_runtime_t& other)			   = delete;
		game_editor_plugin_runtime_t& operator=(const game_editor_plugin_runtime_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool load(const char* path, string_t& diagnostics);
		bool unload(string_t& diagnostics);
		bool deactivate(string_t& diagnostics);
		bool stage(const char* path, string_t& diagnostics);
		bool activate_staged(string_t& diagnostics);
		bool discard_staged(string_t& diagnostics);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick_logic(world_t& world, f32 dt);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		const game_editor_plugin_callbacks_t& get_callbacks() const
		{
			return _callbacks;
		}

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

		const plugin_type_schema_t& get_schema() const
		{
			return *_schema;
		}

		const plugin_type_schema_t& get_staged_schema() const
		{
			return *_staged_schema;
		}

	private:
		unique_t<plugin_type_schema_t> _schema			 = {};
		unique_t<plugin_type_schema_t> _staged_schema	 = {};
		game_editor_plugin_callbacks_t _staged_callbacks = {};
		string_t					   _staged_path		 = {};
		void*						   _staged_library	 = nullptr;
		game_editor_plugin_callbacks_t _callbacks		 = {};
		string_t					   _path;
		void*						   _library = nullptr;
	};
}
