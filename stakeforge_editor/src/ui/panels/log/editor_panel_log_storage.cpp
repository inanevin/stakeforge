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

namespace sfg
{
	void editor_panel_log_t::on_log(log_level level, const char* msg, void*)
	{
		LOCK_GUARD(_log_storage_mtx);
		_pending_logs.push_back({.text = msg, .level = level});
	}

	void editor_panel_log_t::on_log_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->drain_pending_logs();
	}

	void editor_panel_log_t::drain_pending_logs()
	{
		_drained_logs.clear();
		bool clear_rows = false;
		{
			LOCK_GUARD(_log_storage_mtx);
			append_pending_logs_to_storage();
			clear_rows = _storage_generation != _log_storage_generation;
			if (clear_rows)
			{
				_storage_generation = _log_storage_generation;
				_next_log_sequence	= 0;
			}
			for (const log_record_t& record : _stored_logs)
			{
				if (record.sequence >= _next_log_sequence)
					_drained_logs.push_back(record);
			}
		}

		if (clear_rows)
			clear_log_rows();

		if (_drained_logs.empty())
			return;

		const bool was_at_end = is_scrolled_to_end();
		for (const log_record_t& record : _drained_logs)
		{
			add_log_row(record.level, record.text.c_str());
			_next_log_sequence = record.sequence + 1;
		}

		if (was_at_end)
			_scrollbar.scroll_to_end_y();
	}

	void editor_panel_log_t::install_log_listener()
	{
		if (_log_listener_installed)
			return;

		{
			LOCK_GUARD(_log_storage_mtx);
			_stored_logs.reserve(EDITOR_LOG_PANEL_ROW_CAPACITY);
			_pending_logs.reserve(32);
		}
		log_t::instance().add_listener(EDITOR_LOG_PANEL_LISTENER_ID, on_log, nullptr);
		_log_listener_installed = true;
	}

	void editor_panel_log_t::append_pending_logs_to_storage()
	{
		for (log_record_t& record : _pending_logs)
		{
			record.sequence = _next_stored_log_sequence++;
			_stored_logs.push_back(std::move(record));
		}
		_pending_logs.clear();

		if (_stored_logs.size() > EDITOR_LOG_PANEL_ROW_CAPACITY)
		{
			const size_t excess = _stored_logs.size() - EDITOR_LOG_PANEL_ROW_CAPACITY;
			_stored_logs.erase(_stored_logs.begin(), _stored_logs.begin() + excess);
		}
	}

	void editor_panel_log_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->flush_pending_ui_mutations();
	}
}
