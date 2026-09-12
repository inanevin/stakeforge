/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#pragma once

#include "ui/panels/log/editor_panel_log.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/log.hpp>

namespace sfg
{
#define EDITOR_LOG_PANEL_ROW_CAPACITY	  192
#define EDITOR_LOG_PANEL_AUTO_SCROLL_SLOP 1.0f
#define EDITOR_LOG_PANEL_LISTENER_ID	  0x0E100001u

	static inline const editor_dropdown_item_t LOG_SOURCE_ITEMS[] = {
		{.text = "All", .value = static_cast<u16>(log_source_filter_e::all)},
		{.text = "Engine", .value = static_cast<u16>(log_source_filter_e::engine)},
		{.text = "Game", .value = static_cast<u16>(log_source_filter_e::game)},
	};

	inline const char* log_source_filter_to_string(log_source_filter_e filter)
	{
		switch (filter)
		{
		case log_source_filter_e::all:
			return "all";
		case log_source_filter_e::engine:
			return "engine";
		case log_source_filter_e::game:
			return "game";
		}

		return "all";
	}

	inline log_source_filter_e log_source_filter_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);

		if (id == TO_SID("engine") || id == TO_SID("editor"))
			return log_source_filter_e::engine;

		if (id == TO_SID("game"))
			return log_source_filter_e::game;

		return log_source_filter_e::all;
	}

	inline u8 log_level_to_filter_flag(log_level level)
	{
		switch (level)
		{
		case log_level::error:
			return log_level_filter_err;
		case log_level::warning:
			return log_level_filter_warn;
		case log_level::trace:
			return log_level_filter_trace;
		case log_level::info:
		case log_level::progress:
			return log_level_filter_info;
		}
		return log_level_filter_info;
	}

	inline const char* log_level_to_icon(log_level level)
	{
		switch (level)
		{
		case log_level::error:
			return ICON_ERROR;
		case log_level::warning:
			return ICON_WARN;
		case log_level::trace:
			return ICON_TRACE;
		case log_level::info:
		case log_level::progress:
			return ICON_INFO;
		}
		return ICON_INFO;
	}

	inline vec4f_t log_level_to_color(log_level level, const editor_theme_t& theme)
	{
		switch (level)
		{
		case log_level::error:
			return theme.color_accent_err;
		case log_level::warning:
			return theme.color_accent_warn;
		case log_level::trace:
			return theme.color_accent1;
		case log_level::info:
		case log_level::progress:
			return theme.color_text0;
		}
		return theme.color_text0;
	}
}
