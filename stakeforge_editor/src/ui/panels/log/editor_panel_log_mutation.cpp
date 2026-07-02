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
	bool editor_panel_log_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_panel_log_t::request_clear_logs()
	{
		_clear_logs_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_log_t::request_collapse_rows()
	{
		_collapse_rows_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_log_t::flush_pending_ui_mutations()
	{
		const bool do_clear_logs = _clear_logs_pending;
		const bool collapse_rows = _collapse_rows_pending;
		_clear_logs_pending		 = false;
		_collapse_rows_pending	 = false;

		if (do_clear_logs)
		{
			clear_logs();
			return;
		}

		if (collapse_rows)
			collapse_existing_rows();
	}

}
