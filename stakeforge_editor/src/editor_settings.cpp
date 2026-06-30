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
#include <sfg/vendor/nhlohmann/json.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	bool editor_settings_t::save()
	{
		const nlohmann::json json_data = *this;

		const string_t data		= json_data.dump(4);
		const string_t settings = editor_directories_t::get_editor_settings();
		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), settings.c_str()))
		{
			SFG_ERR("failed file serialization!");
			return false;
		}

		return true;
	}

	bool editor_settings_t::ensure_loaded()
	{
		const string_t settings = editor_directories_t::get_editor_settings();
		if (!file_system_t::exists(settings.c_str()))
		{
			editor_settings_t::get() = {};
			return editor_settings_t::get().save();
		}

		const string_t		 json_text = file_system_t::read_file_as_string(settings.c_str());
		const nlohmann::json json	   = nlohmann::json::parse(json_text, nullptr, false);
		if (json.is_discarded())
			return false;

		*this = json;
		return true;
	}

}

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["layout"]		  = settings.layout;
		j["project_path"] = settings.last_project_path;
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings.layout			   = j.value("layout", editor_layout_t{});
		settings.last_project_path = j.value<string_t>("project_path", {});
	}
}
