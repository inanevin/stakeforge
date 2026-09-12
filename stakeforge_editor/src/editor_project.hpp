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

#include <sfg/common/size_definitions.hpp>
#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/runtime/project/project_settings.hpp>

namespace sfg
{
	struct editor_project_runtime_t
	{
		string_t path;
		string_t assets_path;
		string_t cache_path;
		string_t default_assets_path;
		string_t game_assets_path;
		string_t cook_path;
		string_t intermediate_path;
		string_t csharp_intermediate_path;
		string_t script_project_path;
		string_t library_path;
		string_t script_library_path;
		string_t name;
	};

	struct editor_project_settings_data_t
	{
		project_settings_t project_settings					= {};
		sid_t			   last_world_guid					= NULL_SID;
		bool			   world_view_snapping_enabled		= false;
		bool			   world_view_grid_enabled			= false;
		bool			   world_view_aabb_enabled			= false;
		bool			   world_view_physics_debug_enabled = false;
		bool			   world_view_perf_metrics_enabled	= false;

		bool operator==(const editor_project_settings_data_t&) const = default;
	};

	struct editor_project_t
	{
		static editor_project_t& get()
		{
			static editor_project_t instance;
			return instance;
		}

		editor_project_runtime_t	   _runtime = {};
		editor_project_settings_data_t settings = {};

		bool					save(const char* path);
		bool					try_load(const char* path);
		bool					ensure_script_project();
		void					refresh_runtime(const char* path);
		static editor_project_t make_default_project(const char* path);
	};

	SFG_DEFINE_TYPE_ID(editor_project_settings_data_t);
	SFG_DEFINE_TYPE_ID(editor_project_t);

	struct editor_project_reflection_t
	{
		editor_project_reflection_t();
	};

	inline editor_project_reflection_t g_reflect_editor_project;
}
