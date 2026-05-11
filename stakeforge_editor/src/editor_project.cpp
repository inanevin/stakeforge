// Copyright (c) 2025 Inan Evin

#include "editor_project.hpp"
#include <sfg/io/file_system.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool editor_project_t::is_project_path(const char* path)
	{
		if (path == nullptr || path[0] == '\0')
			return false;
		return file_system_t::get_file_extension(path) == "sfg_project";
	}

	editor_project_t editor_project_t::make_default_project(const char* path)
	{
		editor_project_t project	= {};
		project.path				= path;
		project.name				= file_system_t::remove_extensions_from_path(file_system_t::get_filename_and_extension_from_path(project.path));
		project.last_world_resource = {};
		return project;
	}

	void to_json(nlohmann::json& j, const editor_project_t& project)
	{
		j["path"]				 = project.path;
		j["name"]				 = project.name;
		j["last_world_resource"] = project.last_world_resource;
	}

	void from_json(const nlohmann::json& j, editor_project_t& project)
	{
		project.path				= j.value<string_t>("path", {});
		project.name				= j.value<string_t>("name", {});
		project.last_world_resource = j.value<string_t>("last_world_resource", {});
	}
}
