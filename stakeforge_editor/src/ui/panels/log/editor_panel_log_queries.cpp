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
#include "ui/panels/log/editor_panel_log.hpp"
#include "ui/panels/log/editor_panel_log_internal.hpp"

#include <sfg/data/frame_string.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	bool editor_panel_log_t::is_log_row_visible(const log_row_t& row) const
	{
		frame_string_t<char> lower_case_raw = {};
		lower_case_raw.assign(row.raw_text.c_str(), row.raw_text.size());
		string_util::to_lower(lower_case_raw);

		const bool source_visible = _source_filter == log_source_filter_e::all || (_source_filter == log_source_filter_e::engine && row.source == log_source_e::engine) || (_source_filter == log_source_filter_e::game && row.source == log_source_e::game);

		return source_visible && (_log_filter_flags & row.flag) != 0 && lower_case_raw.find(_search_text_lower.c_str()) != frame_string_t<char>::npos;
	}

	bool editor_panel_log_t::is_scrolled_to_end() const
	{
		const ui::layout_in_t&	in	= _ui->get_tree().in_const(_body);
		const ui::layout_out_t& out = _ui->get_tree().out(_body);
		return out.max_scroll.y <= 0.0f || in.scroll_offset.y <= -out.max_scroll.y + EDITOR_LOG_PANEL_AUTO_SCROLL_SLOP;
	}

}
