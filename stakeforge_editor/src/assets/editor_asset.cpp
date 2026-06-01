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

#include "editor_project.hpp"

#include <sfg/data/char_util.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	bool editor_asset_util_t::read_asset(const char* path, editor_asset_t& out_asset)
	{
		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse asset {0}", path);
			return false;
		}

		doc.get_to(out_asset);
		return true;
	}

	bool editor_asset_util_t::write_asset(const char* path, const editor_asset_t& asset)
	{
		const nlohmann::json json_data = asset;
		const string_t		 data	   = json_data.dump(4);
		return serializer_t::write_to_file(string_view_t(data.data(), data.size()), path);
	}

	string_t editor_asset_util_t::normalize_directory(const char* directory)
	{
		string_t result = directory != nullptr ? directory : "";
		if (!result.empty() && result.back() != '/')
			result += '/';
		return result;
	}

	string_t editor_asset_util_t::make_asset_path(const char* directory, const char* asset_name)
	{
		string_t result = normalize_directory(directory);
		result += asset_name != nullptr ? asset_name : "";
		result += ".sfg_asset";
		return result;
	}

	string_t editor_asset_util_t::make_unique_source_path(const char* directory, const char* file_name, const char* extension)
	{
		string_t base = normalize_directory(directory);
		base += file_name != nullptr ? file_name : "";

		string_t ext = extension != nullptr ? extension : "";
		if (!ext.empty() && ext[0] != '.')
			ext.insert(ext.begin(), '.');

		string_t result			 = base + ext;
		size_t	 insert_position = base.size();
		while (file_system_t::exists(result.c_str()))
		{
			result.insert(insert_position, " (Copy)");
			insert_position += 7;
		}

		return result;
	}

	string_t editor_asset_util_t::get_cache_path_for_asset(const editor_asset_t& asset)
	{
		string_t result = editor_project_t::get()._runtime.cache_path;
		file_system_t::fix_path(result);
		file_system_t::fix_path_end_slash(result);

		char  guid_text[32] = {};
		char* guid_text_cur = guid_text;
		if (!char_util::append_u64(guid_text_cur, guid_text + sizeof(guid_text), asset.guid))
			SFG_ASSERT(false);

		result += guid_text;
		result += ".sfg_bin";
		return result;
	}

	string_t editor_asset_util_t::get_source_full_path(const char* assets_path, const editor_asset_t& asset)
	{
		SFG_ASSERT(!asset.source_relative.empty());

		string_t result = file_system_t::get_absolute_path(assets_path);
		file_system_t::fix_path_end_slash(result);
		result += asset.source_relative;
		file_system_t::fix_path(result);
		SFG_ASSERT(file_system_t::exists(result.c_str()));
		return result;
	}

	string_t editor_asset_util_t::get_source_relative(const char* assets_path, const char* source_full_path)
	{
		if (source_full_path == nullptr || source_full_path[0] == '\0')
			return {};

		string_t normalized_assets_path = file_system_t::get_absolute_path(assets_path);
		file_system_t::fix_path_end_slash(normalized_assets_path);
		string_t normalized_assets_path_lower = normalized_assets_path;
		string_util::to_lower(normalized_assets_path_lower);

		const string_t normalized_source_path		= file_system_t::get_absolute_path(source_full_path);
		string_t	   normalized_source_path_lower = normalized_source_path;
		string_util::to_lower(normalized_source_path_lower);
		if (normalized_source_path_lower.rfind(normalized_assets_path_lower, 0) != 0)
			return {};

		return file_system_t::get_relative(normalized_assets_path.c_str(), normalized_source_path.c_str());
	}

	bool editor_asset_util_t::is_source_inside_assets(const char* assets_path, const char* source_full_path)
	{
		return !get_source_relative(assets_path, source_full_path).empty();
	}

	void editor_asset_util_t::fetch_dependencies(const editor_asset_t& asset, frame_vector_t<sid_t>& out_dependencies)
	{
		const auto push_dependency = [&](sid_t dependency) {
			if (dependency != NULL_SID)
				out_dependencies.push_back(dependency);
		};

		switch (asset.asset_type)
		{
		case editor_asset_type_e::mesh:
		case editor_asset_type_e::material: {
			if (!asset.embedded_source.is_object())
				break;

			push_dependency(asset.embedded_source.value<sid_t>("shader", NULL_SID));
			push_dependency(asset.embedded_source.value<sid_t>("sampler", NULL_SID));
			const vector_t<sid_t> textures = asset.embedded_source.value<vector_t<sid_t>>("textures", {});
			out_dependencies.reserve(out_dependencies.size() + textures.size());
			for (const sid_t texture : textures)
				push_dependency(texture);
			break;
		}
		default:
			break;
		}
	}

	sid_t editor_asset_util_t::try_read_existing_guid(const char* path)
	{
		editor_asset_t asset = {};
		return read_asset(path, asset) ? asset.guid : NULL_SID;
	}

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
		case editor_asset_source_type_e::data:
			j = "data";
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
		else if (s == "data")
			t = editor_asset_source_type_e::data;
		else
			t = editor_asset_source_type_e::file;
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
}
