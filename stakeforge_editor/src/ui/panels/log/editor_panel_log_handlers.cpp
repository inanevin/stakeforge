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
#include <sfg/data/string_util.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_log_t::on_source_pressed(u16 value, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._source_filter	  = static_cast<log_source_filter_e>(value);
		panel.refresh_log_filter_visibility();
	}

	void editor_panel_log_t::on_filter_pressed(bool toggled, void* user_data)
	{
		const log_filter_button_data_t& data = *static_cast<log_filter_button_data_t*>(user_data);
		if (toggled)
			data.panel->_log_filter_flags |= data.flag;
		else
			data.panel->_log_filter_flags &= static_cast<u8>(~data.flag);
		data.panel->refresh_log_filter_visibility();
	}

	void editor_panel_log_t::on_collapse_pressed(bool toggled, void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._is_collapsed		  = toggled;
		if (toggled)
			panel.request_collapse_rows();
	}

	void editor_panel_log_t::on_clear_pressed(bool, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->request_clear_logs();
	}

	void editor_panel_log_t::on_search_changed(void* user_data)
	{
		editor_panel_log_t& panel = *static_cast<editor_panel_log_t*>(user_data);
		panel._search_text_lower  = panel._search_text;
		string_util::to_lower(panel._search_text_lower);
		panel.refresh_log_filter_visibility();
	}

}
