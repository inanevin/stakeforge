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
