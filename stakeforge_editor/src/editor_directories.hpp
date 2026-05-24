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

#include <sfg/data/string.hpp>

namespace sfg
{
	struct editor_project_t;

	class editor_directories_t
	{
	public:
		static inline const string_t& get_editor_resource_cache()
		{
			return s_editor_resource_cache;
		}

		static inline const string_t& get_editor_manifest()
		{
			return s_editor_manifest;
		}

		static inline const string_t& get_user_directory()
		{
			return s_user_directory;
		}

		static inline const string_t& get_editor_assets()
		{
			return s_editor_assets;
		}

		static inline const string_t& get_editor_settings()
		{
			return s_editor_settings;
		}

		static bool is_valid_asset_name(const char* name);

	private:
		friend class editor_app_t;

		static string_t s_user_directory;
		static string_t s_editor_settings;
		static string_t s_editor_assets;
		static string_t s_editor_resource_cache;
		static string_t s_editor_manifest;

		static void init_paths();
	};
}
