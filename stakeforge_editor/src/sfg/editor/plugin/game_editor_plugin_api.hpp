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

#include <sfg/core/common/size_definitions.hpp>

#define SFG_GAME_EDITOR_PLUGIN_ENTRY extern "C" __declspec(dllexport)

namespace sfg
{
	class world_t;
	class plugin_type_schema_t;

	inline constexpr u32 GAME_EDITOR_PLUGIN_API_VERSION = 12;

	struct game_editor_plugin_callbacks_t
	{
		void (*register_types)(plugin_type_schema_t& schema) = nullptr;
		void (*load)()										 = nullptr;
		void (*unload)()									 = nullptr;
		void (*tick_logic)(world_t& world, f32 dt)			 = nullptr;
	};

	using game_editor_plugin_entry_fn = bool (*)(u32 api_version, game_editor_plugin_callbacks_t& callbacks);
}
