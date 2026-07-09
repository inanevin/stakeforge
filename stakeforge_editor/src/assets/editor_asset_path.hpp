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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_path_t final
	{
	public:
		editor_asset_path_t()									   = delete;
		~editor_asset_path_t()									   = delete;
		editor_asset_path_t(const editor_asset_path_t&)			   = delete;
		editor_asset_path_t& operator=(const editor_asset_path_t&) = delete;

		static string_t normalize_directory(const char* directory);
		static string_t make_asset_path(const char* directory, const char* asset_name);
		static string_t make_blob_path(const char* directory, const char* asset_name);
		static string_t make_source_path(const char* directory, const char* file_name, const char* extension);
		static string_t make_unique_source_path(const char* directory, const char* file_name, const char* extension);
		static string_t get_cache_path_for_guid(sid_t guid);
		static string_t get_source_full_path(const char* assets_path, const editor_asset_t& asset);
		static string_t get_source_relative(const char* assets_path, const char* source_full_path);
		static bool		is_source_inside_assets(const char* assets_path, const char* source_full_path);
		static u64		hash_path(const char* path);
	};
}
