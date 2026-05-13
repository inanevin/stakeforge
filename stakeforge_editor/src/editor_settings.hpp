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

#include "editor_project.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_window_settings_t
	{
		vec2i16_t position		= {64, 64};
		vec2u16_t size			= {1920, 1080};
		u64		  monitor_ident = UINT64_MAX;
	};

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

		u16	 add_window(const editor_window_settings_t& w);
		void remove_window(u16 index);

		inline editor_window_settings_t& get_window(u16 index)
		{
			return _windows[index];
		}

		inline const editor_window_settings_t& get_window(u16 index) const
		{
			return _windows[index];
		}

		inline span_t<const editor_window_settings_t> get_windows() const
		{
			return {_windows.data(), _windows.size()};
		}

		inline u16 get_window_count() const
		{
			return static_cast<u16>(_windows.size());
		}
		inline editor_project_t& get_project()
		{
			return _project;
		}
		inline const editor_project_t& get_project() const
		{
			return _project;
		}

	private:
		void flush_to_disk();

		editor_settings_t()									   = default;
		~editor_settings_t()								   = default;
		editor_settings_t(const editor_settings_t&)			   = delete;
		editor_settings_t& operator=(const editor_settings_t&) = delete;

		vector_t<editor_window_settings_t> _windows;
		editor_project_t				   _project;

		friend void to_json(nlohmann::json& j, const editor_settings_t& settings);
		friend void from_json(const nlohmann::json& j, editor_settings_t& settings);
	};

	void to_json(nlohmann::json& j, const editor_window_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_window_settings_t& settings);
	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
