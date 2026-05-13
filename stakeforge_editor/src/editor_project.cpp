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
