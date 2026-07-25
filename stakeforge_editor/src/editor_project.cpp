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
#include "editor_project.hpp"
#include <sfg/io/file_system.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <cstddef>

namespace sfg
{
	editor_project_t editor_project_t::make_default_project(const char* path)
	{
		editor_project_t project = {};
		project.refresh_runtime(path);
		project.settings.last_world_guid = NULL_SID;
		return project;
	}

	bool editor_project_t::save(const char* path)
	{
		nlohmann::json js = nlohmann::json::object();
		if (!reflection_registry_t::get().type_to_json(type_id_t<editor_project_t>::value, this, nullptr, js))
			return false;
		const string_t contents = js.dump(4);
		return serializer_t::write_to_file(contents, path);
	}

	bool editor_project_t::try_load(const char* path)
	{
		if (!file_system_t::exists(path))
			return false;

		const string_t&		 contents = file_system_t::read_file_as_string(path);
		const nlohmann::json js		  = nlohmann::json::parse(contents, nullptr, false);
		if (js.is_discarded())
			return false;

		editor_project_t project = make_default_project(path);
		if (!reflection_registry_t::get().type_from_json(type_id_t<editor_project_t>::value, &project, nullptr, js))
			return false;

		*this = project;
		settings.project_settings.normalize();
		refresh_runtime(path);
		engine_runtime_t::get().update_project_settings(settings.project_settings);
		return true;
	}

	void editor_project_t::refresh_runtime(const char* path)
	{
		_runtime = {};

		_runtime.path = path;
		file_system_t::fix_path(_runtime.path);
		_runtime.name	   = file_system_t::get_filename_from_path(path);
		const string_t dir = file_system_t::get_directory_of_file(_runtime.path.c_str());

		_runtime.assets_path		 = dir + "assets/";
		_runtime.cache_path			 = _runtime.assets_path + "_cache/";
		_runtime.default_assets_path = _runtime.assets_path + "_sfg_assets/";

		if (!file_system_t::exists(_runtime.assets_path.c_str()))
			file_system_t::create_directory(_runtime.assets_path.c_str());

		if (!file_system_t::exists(_runtime.cache_path.c_str()))
			file_system_t::create_directory(_runtime.cache_path.c_str());

		if (!file_system_t::exists(_runtime.default_assets_path.c_str()))
			file_system_t::create_directory(_runtime.default_assets_path.c_str());
	}

}

namespace sfg
{
	editor_project_reflection_t::editor_project_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		registry.register_type({
			.name		  = "editor_project_settings_data_t",
			.display_name = "Editor Project Settings",
			.fields =
				{
					{.name = "last_world_guid", .display_name = "Last World GUID", .offset = offsetof(editor_project_settings_data_t, last_world_guid), .size = sizeof(sid_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name		   = "world_view_snapping_enabled",
					 .display_name = "World View Snapping",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_snapping_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_grid_enabled",
					 .display_name = "World View Grid",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_grid_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_aabb_enabled",
					 .display_name = "World View AABB",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_aabb_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_skeleton_enabled",
					 .display_name = "World View Skeleton",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_skeleton_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "world_view_physics_debug_enabled",
					 .display_name = "World View Physics Debug",
					 .offset	   = offsetof(editor_project_settings_data_t, world_view_physics_debug_enabled),
					 .size		   = sizeof(bool),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::boolean},
					{.name		   = "project_settings",
					 .display_name = "Project",
					 .sub_type_id  = type_id_t<project_settings_t>::value,
					 .offset	   = offsetof(editor_project_settings_data_t, project_settings),
					 .size		   = sizeof(project_settings_t),
					 .type		   = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_settings_data_t>::value,
			.size	   = sizeof(editor_project_settings_data_t),
			.alignment = alignof(editor_project_settings_data_t),
		});
		registry.register_type({
			.name		  = "editor_project_t",
			.display_name = "Project Settings",
			.fields =
				{
					{.name		   = "settings",
					 .display_name = "Settings",
					 .sub_type_id  = type_id_t<editor_project_settings_data_t>::value,
					 .offset	   = offsetof(editor_project_t, settings),
					 .size		   = sizeof(editor_project_settings_data_t),
					 .type		   = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<editor_project_t>::value,
			.size	   = sizeof(editor_project_t),
			.alignment = alignof(editor_project_t),
		});
	}
}
