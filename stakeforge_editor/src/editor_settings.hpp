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

#include "editor_layout.hpp"
#include "editor_project.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class editor_settings_t
	{
	public:
		inline static editor_settings_t& get()
		{
			static editor_settings_t instance;
			return instance;
		}

		bool reload();
		void save();

		inline editor_project_t& get_project()
		{
			return _project;
		}
		inline const editor_project_t& get_project() const
		{
			return _project;
		}
		inline editor_layout_t& get_layout()
		{
			return _layout;
		}
		inline const editor_layout_t& get_layout() const
		{
			return _layout;
		}

	private:
		void flush_to_disk();

		editor_settings_t()									   = default;
		~editor_settings_t()								   = default;
		editor_settings_t(const editor_settings_t&)			   = delete;
		editor_settings_t& operator=(const editor_settings_t&) = delete;

		editor_layout_t	 _layout;
		editor_project_t _project;

		friend void to_json(nlohmann::json& j, const editor_settings_t& settings);
		friend void from_json(const nlohmann::json& j, editor_settings_t& settings);
	};

	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
