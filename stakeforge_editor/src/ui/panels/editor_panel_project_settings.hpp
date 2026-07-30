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
		void request_refresh_reflection();
		void flush_pending_ui_mutations();
		bool can_mutate_ui_topology() const;
		void begin_project_settings_edit();
		void submit_project_settings_edit();

		static void on_project_settings_edit_begin(void* user_data);
		static void on_project_settings_edit_submitted(void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		vector_t<editor_widget_reflection_fold_state_t> _field_states		   = {};
		editor_widget_reflection_t						_reflection			   = {};
		editor_widget_reflection_t						_editor_reflection	   = {};
		editor_scrollbar_t								_scrollbar			   = {};
		editor_command_project_settings_data_t			_project_edit_previous = {};
		editor_command_listener_handle_t				_command_listener	   = {};
		ui::widget_id_t									_scroll_area		   = NULL_WIDGET;
		ui::widget_id_t									_content			   = NULL_WIDGET;
		bool											_refresh_pending	   = false;
		bool											_project_edit_active   = false;
	};
}
