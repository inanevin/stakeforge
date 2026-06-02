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

#include "editor_asset_reflection.hpp"
#include "editor_asset_type_reflection.hpp"
#include <sfg/data/string.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t)
		{
			const string_t s = j.get<string_t>();
	
			if (s == "none")
				t = editor_asset_source_type_e::none;
			else if (s == "embedded")
				t = editor_asset_source_type_e::embedded;
			else if (s == "data")
				t = editor_asset_source_type_e::data;
			else
				t = editor_asset_source_type_e::file;
		}

	void from_json(const nlohmann::json& j, editor_asset_t& asset)
		{
			asset.version		  = j.value<u32>("version", editor_asset_t::VERSION);
			asset.guid			  = j.value<sid_t>("guid", NULL_SID);
			asset.asset_type	  = j.value<editor_asset_type_e>("asset_type", j.value<editor_asset_type_e>("resource_type", j.value<editor_asset_type_e>("type", editor_asset_type_e::invalid)));
			asset.sub_type		  = j.value<u8>("sub_type", 0);
			asset.embedded_source = j.value<nlohmann::json>("embedded_source", nlohmann::json());
			asset.cook_options	  = j.value<nlohmann::json>("cook_options", nlohmann::json::object());
			asset.source_relative = j.value<string_t>("source_relative", {});
			asset.source_type	  = j.value<editor_asset_source_type_e>("source_type", editor_asset_source_type_e::file);
			asset.status		  = editor_asset_status_e::ok;
			asset._transient_data = {};
		}

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t)
		{
			switch (t)
			{
			case editor_asset_source_type_e::none:
				j = "none";
				break;
			case editor_asset_source_type_e::embedded:
				j = "embedded";
				break;
			case editor_asset_source_type_e::data:
				j = "data";
				break;
			default:
				j = "file";
				break;
			}
		}

	void to_json(nlohmann::json& j, const editor_asset_t& asset)
		{
			j["version"]		 = asset.version;
			j["guid"]			 = asset.guid;
			j["asset_type"]		 = asset.asset_type;
			j["sub_type"]		 = asset.sub_type;
			j["embedded_source"] = asset.embedded_source;
			j["cook_options"]	 = asset.cook_options;
			j["source_relative"] = asset.source_relative;
			j["source_type"]	 = asset.source_type;
		}

}
