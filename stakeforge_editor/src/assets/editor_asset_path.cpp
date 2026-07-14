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

#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/char_util.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>

namespace sfg
{
	string_t editor_asset_path_t::normalize_directory(const char* directory)
	{
		string_t result = directory != nullptr ? directory : "";
		if (!result.empty() && result.back() != '/')
			result += '/';
		return result;
	}

	string_t editor_asset_path_t::make_asset_path(const char* directory, const char* asset_name)
	{
		string_t result = normalize_directory(directory);
		result += asset_name != nullptr ? asset_name : "";
		result += ".sfg_asset";
		return result;
	}

	string_t editor_asset_path_t::make_blob_path(const char* directory, const char* asset_name)
	{
		string_t result = normalize_directory(directory);
		result += asset_name != nullptr ? asset_name : "";
		result += ".sfg_bin";
		return result;
	}

	string_t editor_asset_path_t::make_source_path(const char* directory, const char* file_name, const char* extension)
	{
		string_t result = normalize_directory(directory);
		result += file_name != nullptr ? file_name : "";

		string_t ext = extension != nullptr ? extension : "";
		if (!ext.empty() && ext[0] != '.')
			ext.insert(ext.begin(), '.');

		result += ext;
		return result;
	}

	string_t editor_asset_path_t::make_unique_source_path(const char* directory, const char* file_name, const char* extension)
	{
		string_t	   result		   = make_source_path(directory, file_name, extension);
		size_t		   insert_position = result.size();
		const string_t ext			   = extension != nullptr ? extension : "";
		if (!ext.empty())
			insert_position -= ext[0] == '.' ? ext.size() : ext.size() + 1;
		while (file_system_t::exists(result.c_str()))
		{
			result.insert(insert_position, " (Copy)");
			insert_position += 7;
		}

		return result;
	}

	string_t editor_asset_path_t::get_cache_path_for_guid(sid_t guid)
	{
		string_t result = editor_project_t::get()._runtime.cache_path;

		char  guid_text[32] = {};
		char* guid_text_cur = guid_text;
		if (!char_util::append_u64(guid_text_cur, guid_text + sizeof(guid_text), guid))
			SFG_ASSERT(false);

		result += guid_text;
		result += ".sfg_bin";
		return result;
	}

	string_t editor_asset_path_t::get_source_full_path(const char* assets_path, const editor_asset_t& asset)
	{
		string_t result = file_system_t::get_absolute_path(assets_path);
		result += asset.source_relative;
		return result;
	}

	string_t editor_asset_path_t::get_source_relative(const char* assets_path, const char* source_full_path)
	{
		if (source_full_path == nullptr || source_full_path[0] == '\0')
			return {};

		string_t normalized_assets_path		  = file_system_t::get_absolute_path(assets_path);
		string_t normalized_assets_path_lower = normalized_assets_path;
		string_util::to_lower(normalized_assets_path_lower);

		const string_t normalized_source_path		= file_system_t::get_absolute_path(source_full_path);
		string_t	   normalized_source_path_lower = normalized_source_path;
		string_util::to_lower(normalized_source_path_lower);
		if (normalized_source_path_lower.rfind(normalized_assets_path_lower, 0) != 0)
			return {};

		return file_system_t::get_relative(normalized_assets_path.c_str(), normalized_source_path.c_str());
	}

	bool editor_asset_path_t::set_source_relative_or_copy(editor_asset_t& asset, const char* asset_directory, const char* asset_name, const char* source_full_path)
	{
		const string_t source_path = file_system_t::get_absolute_path(source_full_path);
		const string_t assets_path = editor_project_t::get()._runtime.assets_path;
		asset.source_relative	   = get_source_relative(assets_path.c_str(), source_path.c_str());
		if (!asset.source_relative.empty())
			return true;

		const string_t source_extension	  = file_system_t::get_file_extension(source_path);
		const string_t target_source_path = make_source_path(asset_directory, asset_name, source_extension.c_str());
		if (!file_system_t::copy_file(source_path.c_str(), target_source_path.c_str()))
			return false;

		asset.source_relative = get_source_relative(assets_path.c_str(), target_source_path.c_str());
		return true;
	}

	bool editor_asset_path_t::is_source_inside_assets(const char* assets_path, const char* source_full_path)
	{
		return !get_source_relative(assets_path, source_full_path).empty();
	}

	u64 editor_asset_path_t::hash_path(const char* path)
	{
		string_t normalized = file_system_t::get_absolute_path(path);
		string_util::to_lower(normalized);
		return hashing_t::hash_u64(normalized.c_str(), normalized.size());
	}
}
