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
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	editor_project_t editor_project_t::make_default_project(const char* path)
	{
		editor_project_t project = {};
		project.refresh_runtime(path);
		project.last_world_guid	   = NULL_SID;
		project.world_tick_rate	   = 60;
		project.world_physics_rate = 100;
		project.max_sim_steps	   = 4;
		return project;
	}

	bool editor_project_t::save(const char* path)
	{
		const nlohmann::json js		  = *this;
		const string_t		 contents = js.dump(4);
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

		*this = js;
		refresh_runtime(path);
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
	void to_json(nlohmann::json& j, const editor_project_t& project)
	{
		j["last_world_guid"]	= project.last_world_guid;
		j["world_tick_rate"]	= project.world_tick_rate;
		j["world_physics_rate"] = project.world_physics_rate;
		j["max_sim_steps"]		= project.max_sim_steps;
	}

	void from_json(const nlohmann::json& j, editor_project_t& project)
	{
		project.last_world_guid	   = j.value<sid_t>("last_world_guid", NULL_SID);
		project.world_tick_rate	   = j.value<u32>("world_tick_rate", 60);
		project.world_physics_rate = j.value<u32>("world_physics_rate", 100);
		project.max_sim_steps	   = j.value<u32>("max_sim_steps", 4);
	}
}
