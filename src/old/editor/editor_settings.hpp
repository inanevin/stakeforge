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

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "math/vector2.hpp"
#include <vendor/nhlohmann/json_fwd.hpp>

namespace SFG
{
	class window;

	struct editor_settings
	{
		string	last_world_relative = "";
		string	working_dir			= "";
		string	cache_dir			= "";
		string	_editor_folder		= "";
		string	_resource_cache		= "";
		vector2 window_pos			= vector2::zero;
		vector2 window_size			= vector2(1024, 1024);

		static editor_settings& get()
		{
			static editor_settings e;
			return e;
		}

		bool   init();
		void   init_window_layout(window& wnd);
		void   uninit();
		bool   load(const char* path);
		bool   save(const char* path);
		bool   is_in_work_directory(const string& path) const;
		string get_relative(const string& path) const;

		inline bool save_last()
		{
			return save(_last_path.c_str());
		}

		string _last_path = "";
	};

	void to_json(nlohmann::json& j, const editor_settings& t);
	void from_json(const nlohmann::json& j, editor_settings& s);
}
