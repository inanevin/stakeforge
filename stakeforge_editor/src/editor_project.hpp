/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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
		bool			   world_view_skeleton_enabled		= false;
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
