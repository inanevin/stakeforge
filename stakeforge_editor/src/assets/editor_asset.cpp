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

#include "assets/editor_asset.hpp"

#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_asset_type_e& t)
	{
		switch (t)
		{
		case editor_asset_type_e::audio:
			j = "audio";
			break;
		case editor_asset_type_e::font:
			j = "font";
			break;
		case editor_asset_type_e::mesh:
			j = "mesh";
			break;
		case editor_asset_type_e::skeleton:
			j = "skeleton";
			break;
		case editor_asset_type_e::animation:
			j = "animation";
			break;
		case editor_asset_type_e::particle_properties:
			j = "particle_properties";
			break;
		case editor_asset_type_e::material:
			j = "material";
			break;
		case editor_asset_type_e::shader:
			j = "shader";
			break;
		case editor_asset_type_e::texture:
			j = "texture";
			break;
		case editor_asset_type_e::texture_sampler:
			j = "texture_sampler";
			break;
		case editor_asset_type_e::physical_material:
			j = "physical_material";
			break;
		case editor_asset_type_e::prefab:
			j = "prefab";
			break;
		case editor_asset_type_e::animation_state_machine:
			j = "animation_state_machine";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_asset_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "audio")
			t = editor_asset_type_e::audio;
		else if (s == "font")
			t = editor_asset_type_e::font;
		else if (s == "mesh")
			t = editor_asset_type_e::mesh;
		else if (s == "skeleton")
			t = editor_asset_type_e::skeleton;
		else if (s == "animation")
			t = editor_asset_type_e::animation;
		else if (s == "particle_properties")
			t = editor_asset_type_e::particle_properties;
		else if (s == "material")
			t = editor_asset_type_e::material;
		else if (s == "shader")
			t = editor_asset_type_e::shader;
		else if (s == "texture")
			t = editor_asset_type_e::texture;
		else if (s == "texture_sampler")
			t = editor_asset_type_e::texture_sampler;
		else if (s == "physical_material")
			t = editor_asset_type_e::physical_material;
		else if (s == "prefab")
			t = editor_asset_type_e::prefab;
		else if (s == "animation_state_machine")
			t = editor_asset_type_e::animation_state_machine;
		else
			t = editor_asset_type_e::invalid;
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
		default:
			j = "file";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "none")
			t = editor_asset_source_type_e::none;
		else if (s == "embedded")
			t = editor_asset_source_type_e::embedded;
		else
			t = editor_asset_source_type_e::file;
	}

	void to_json(nlohmann::json& j, const editor_asset_t& asset)
	{
		j["guid"]				  = asset.guid;
		j["asset_type"]			  = asset.asset_type;
		j["source_type"]		  = asset.source_type;
		j["source_relative_path"] = asset.source_relative_path;
		j["embedded_source"]	  = asset.embedded_source;
	}

	void from_json(const nlohmann::json& j, editor_asset_t& asset)
	{
		asset.guid				   = j.value<sid_t>("guid", 0);
		asset.asset_type		   = j.value<editor_asset_type_e>("asset_type", j.value<editor_asset_type_e>("resource_type", j.value<editor_asset_type_e>("type", editor_asset_type_e::invalid)));
		asset.source_type		   = j.value<editor_asset_source_type_e>("source_type", editor_asset_source_type_e::none);
		asset.source_relative_path = j.value<string_t>("source_relative_path", {});
		asset.embedded_source	   = j.value<string_t>("embedded_source", {});
	}
}
