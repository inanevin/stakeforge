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

#include "commands/editor_command_project_settings.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"

namespace sfg
{
	class editor_panel_project_settings_t final : public editor_panel_t
	{
	public:
		editor_panel_project_settings_t();
		~editor_panel_project_settings_t() override										   = default;
		editor_panel_project_settings_t(const editor_panel_project_settings_t&)			   = delete;
		editor_panel_project_settings_t& operator=(const editor_panel_project_settings_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

	private:
		void refresh_reflection();
		void begin_project_settings_edit();
		void submit_project_settings_edit();

		static void on_project_settings_edit_begin(void* user_data);
		static void on_project_settings_edit_submitted(void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		vector_t<editor_widget_reflection_fold_state_t> _field_states		   = {};
		editor_widget_reflection_t						_reflection			   = {};
		editor_widget_reflection_t						_editor_reflection	   = {};
		editor_scrollbar_t								_scrollbar			   = {};
		editor_command_project_settings_data_t			_project_edit_previous = {};
		editor_command_listener_handle_t				_command_listener	   = {};
		ui::widget_id_t									_scroll_area		   = NULL_WIDGET;
		ui::widget_id_t									_content			   = NULL_WIDGET;
		bool											_project_edit_active   = false;
	};
}
