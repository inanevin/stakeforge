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
#include "editor_layout.hpp"
#include "editor_surface.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_layout_t::load_surface_default_layout(editor_surface_t& surface)
	{
		nlohmann::json layout = {
			{"version", 1},
			{"root",
			 nlohmann::json{
				 {"type", "split"},
				 {"direction", "horizontal"},
				 {"split_value", 0.22f},
				 {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Entities"}, {"data", nlohmann::json::object()}}})}}},
				 {"positive",
				  nlohmann::json{
					  {"type", "split"},
					  {"direction", "horizontal"},
					  {"split_value", 0.72f},
					  {"negative",
					   nlohmann::json{
						   {"type", "split"},
						   {"direction", "vertical"},
						   {"split_value", 0.68f},
						   {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "World"}, {"data", nlohmann::json::object()}}})}}},
						   {"positive",
							nlohmann::json{
								{"type", "leaf"},
								{"panels",
								 nlohmann::json::array(
									 {nlohmann::json{{"type", "Assets"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Log"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Resources"}, {"data", nlohmann::json::object()}}})}}},
					   }},
					  {"positive", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Inspector"}, {"data", nlohmann::json::object()}}})}}},
				  }},
			 }},
		};

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->get_dock_widget().from_json(layout);
	}
}

namespace sfg
{
	namespace
	{
		vec2u16_t vec2u16_from_json(const nlohmann::json& j, const vec2u16_t& fallback)
		{
			if (!j.is_array() || j.size() < 2)
				return fallback;

			return {j.at(0).get<u16>(), j.at(1).get<u16>()};
		}
	}

	void to_json(nlohmann::json& j, const editor_layout_window_t& window)
	{
		const nlohmann::json dock_layout  = nlohmann::json::parse(window.dock_layout, nullptr, false);
		const nlohmann::json main_toolbar = nlohmann::json::parse(window.main_toolbar, nullptr, false);
		j["pos"]						  = nlohmann::json::array_t({window.pos.x, window.pos.y});
		j["size"]						  = nlohmann::json::array_t({window.size.x, window.size.y});
		j["is_primary"]					  = window.is_primary;
		j["maximized"]					  = window.maximized;
		j["dock_layout"]				  = dock_layout.is_object() ? dock_layout : nlohmann::json::object();
		j["main_toolbar"]				  = main_toolbar.is_object() ? main_toolbar : nlohmann::json::object();
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

		window.size		  = vec2u16_from_json(j.value("size", nlohmann::json::array()), vec2u16_t{1920, 1080});
		window.is_primary = j.value("is_primary", false);
		window.maximized  = j.value("maximized", false);

		const nlohmann::json dock_layout = j.value("dock_layout", nlohmann::json::object());
		window.dock_layout				 = dock_layout.is_object() ? string_t(dock_layout.dump()) : string_t("{}");

		const nlohmann::json main_toolbar = j.value("main_toolbar", nlohmann::json::object());
		window.main_toolbar				  = main_toolbar.is_object() ? string_t(main_toolbar.dump()) : string_t("{}");
	}

	void from_json(const nlohmann::json& j, editor_layout_t& layout)
	{
		layout.windows = j.value("windows", vector_t<editor_layout_window_t>{});
	}
}
