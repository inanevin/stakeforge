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

#include "editor_layout_reflection.hpp"
#include <sfg/math/vec2u16_reflection.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_layout_window_t& window)
		{
			const nlohmann::json dock_layout = nlohmann::json::parse(window.dock_layout, nullptr, false);
			j["pos"]						 = nlohmann::json::array_t({window.pos.x, window.pos.y});
			j["size"]						 = window.size;
			j["is_primary"]					 = window.is_primary;
			j["maximized"]					 = window.maximized;
			j["dock_layout"]				 = dock_layout.is_object() ? dock_layout : nlohmann::json::object();
		}

	void to_json(nlohmann::json& j, const editor_layout_t& layout)
		{
			j["windows"] = layout.windows;
		}

	void from_json(const nlohmann::json& j, editor_layout_window_t& window)
		{
			const nlohmann::json pos = j.value("pos", nlohmann::json::array_t({64, 64}));
			if (pos.is_array() && pos.size() >= 2)
				window.pos = {pos.at(0).get<i16>(), pos.at(1).get<i16>()};
	
			window.size		  = j.value("size", vec2u16_t{1920, 1080});
			window.is_primary = j.value("is_primary", false);
			window.maximized  = j.value("maximized", false);
	
			const nlohmann::json dock_layout = j.value("dock_layout", nlohmann::json::object());
			window.dock_layout				 = dock_layout.is_object() ? string_t(dock_layout.dump()) : string_t("{}");
		}

	void from_json(const nlohmann::json& j, editor_layout_t& layout)
		{
			layout.windows = j.value("windows", vector_t<editor_layout_window_t>{});
		}

}
