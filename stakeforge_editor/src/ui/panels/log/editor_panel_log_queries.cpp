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
#include "ui/panels/log/editor_panel_log.hpp"
#include "ui/panels/log/editor_panel_log_internal.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_string.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <cctype>

namespace sfg
{
	bool editor_panel_log_t::is_log_row_visible(const log_row_t& row) const
	{
		frame_string_t<char> lower_case_raw;
		lower_case_raw.assign(row.raw_text.c_str(), row.raw_text.size());
		string_util::to_lower(lower_case_raw);
		return (_log_filter_flags & row.flag) != 0 && lower_case_raw.find(_search_text_lower.c_str()) != frame_string_t<char>::npos;
	}

	bool editor_panel_log_t::is_scrolled_to_end() const
	{
		const ui::layout_in_t&	in	= _ui->get_tree().in_const(_body);
		const ui::layout_out_t& out = _ui->get_tree().out(_body);
		return out.max_scroll.y <= 0.0f || in.scroll_offset.y <= -out.max_scroll.y + EDITOR_LOG_PANEL_AUTO_SCROLL_SLOP;
	}

}
