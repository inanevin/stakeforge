// Copyright (c) 2025 Inan Evin

#include "editor_layout.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_layout_window_t& window)
	{
		const nlohmann::json dock_layout = nlohmann::json::parse(window.dock_layout, nullptr, false);
		j["pos"]						 = nlohmann::json::array_t({window.pos.x, window.pos.y});
		j["size"]						 = window.size;
		j["is_primary"]					 = window.is_primary;
		j["dock_layout"]				 = dock_layout.is_object() ? dock_layout : nlohmann::json::object();
	}

	void from_json(const nlohmann::json& j, editor_layout_window_t& window)
	{
		const nlohmann::json pos = j.value("pos", nlohmann::json::array_t({64, 64}));
		if (pos.is_array() && pos.size() >= 2)
			window.pos = {pos.at(0).get<i16>(), pos.at(1).get<i16>()};

		window.size		  = j.value("size", vec2u16_t{1920, 1080});
		window.is_primary = j.value("is_primary", false);

		const nlohmann::json dock_layout = j.value("dock_layout", nlohmann::json::object());
		window.dock_layout				 = dock_layout.is_object() ? string_t(dock_layout.dump()) : string_t("{}");
	}

	void to_json(nlohmann::json& j, const editor_layout_t& layout)
	{
		j["windows"] = layout.windows;
	}

	void from_json(const nlohmann::json& j, editor_layout_t& layout)
	{
		layout.windows = j.value("windows", vector_t<editor_layout_window_t>{});
	}
}
