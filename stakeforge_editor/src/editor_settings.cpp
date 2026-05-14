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

#include "editor_settings.hpp"
#include "editor_directories.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["layout"]	 = settings._layout;
		j["project"] = settings._project;
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings._layout  = j.value("layout", editor_layout_t{});
		settings._project = j.value("project", editor_project_t{});
	}

	bool editor_settings_t::reload()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system_t::exists(directory.c_str()) && !file_system_t::create_directory(directory.c_str()))
			return false;

		const string_t path = editor_directories_t::get_settings_path();
		if (!file_system_t::exists(path.c_str()))
		{
			_layout = {};
			flush_to_disk();
			return true;
		}

		const string_t		 data = file_system_t::read_file_as_string(path.c_str());
		const nlohmann::json doc  = nlohmann::json::parse(data, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("failed loading editor settings");
			_layout = {};
			flush_to_disk();
			return true;
		}

		doc.get_to(*this);
		return true;
	}

	void editor_settings_t::save()
	{
		flush_to_disk();
	}

	void editor_settings_t::flush_to_disk()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system_t::exists(directory.c_str()) && !file_system_t::create_directory(directory.c_str()))
			return;

		const nlohmann::json json_data = *this;
		const string_t		 data	   = json_data.dump(4);
		serializer_t::write_to_file(string_view_t(data.data(), data.size()), editor_directories_t::get_settings_path().c_str());
	}
}
