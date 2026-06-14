/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions in the documentation and/or other materials provided
	  with the distribution.

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

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	struct editor_surface_t;

	struct editor_layout_window_t
	{
		vec2i16_t pos		 = {64, 64};
		vec2u16_t size		 = {1920, 1080};
		bool	  is_primary = false;
		bool	  maximized	 = false;
		string_t  dock_layout;
		string_t  main_toolbar;
	};

	struct editor_layout_t
	{
		vector_t<editor_layout_window_t> windows;

		static void load_surface_default_layout(editor_surface_t& surface);
	};

	void to_json(nlohmann::json& j, const editor_layout_window_t& window);
	void from_json(const nlohmann::json& j, editor_layout_window_t& window);
	void to_json(nlohmann::json& j, const editor_layout_t& layout);
	void from_json(const nlohmann::json& j, editor_layout_t& layout);
}
