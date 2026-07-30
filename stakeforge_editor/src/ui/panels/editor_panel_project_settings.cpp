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
#include "ui/panels/editor_panel_project_settings.hpp"
#include "editor_project.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_panel_project_settings_t::editor_panel_project_settings_t()
	{
		set_type(editor_panel_type_e::project_settings);
		set_title(editor_panel_type_to_string(editor_panel_type_e::project_settings));
		set_icon(ICON_SETTINGS);
	}

	void editor_panel_project_settings_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		_scroll_area = ui.allocate_widget();
		ui.set_widget_debug_name(_scroll_area, "project_settings_scroll_area");
		tree.attach(_root, _scroll_area);

		ui::layout_in_t& scroll_area_in = tree.in(_scroll_area);
		scroll_area_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		scroll_area_in.child_clip_mode	= ui::clip_mode_e::scissor_rect;
		scroll_area_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_value		= {1.0f, 1.0f};

		_content = ui.allocate_widget();
		ui.set_widget_debug_name(_content, "project_settings_content");
		tree.attach(_scroll_area, _content);
		tree.draw_order(_content) = tree.draw_order_const(_scroll_area) + 1;

		ui::layout_in_t& content_in = tree.in(_content);
		content_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		content_in.size_mode_y		= ui::axis_mode_e::sum_children;
		content_in.size_value		= {1.0f, 1.0f};
		content_in.flow				= ui::flow_e::column;
		content_in.child_spacing	= 0.0f;
		content_in.child_margins	= {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		_scrollbar.init(ui, {.target = _scroll_area, .axes = editor_scrollbar_axis_y});

		void* object = &editor_project_t::get().settings;
		_reflection.init(ui,
						 _content,
						 {
							 .fold_states = &_field_states,
							 .callbacks =
								 {
									 .edit_begin	 = on_project_settings_edit_begin,
									 .edit_submitted = on_project_settings_edit_submitted,
									 .user_data		 = this,
								 },
							 .objects = {.data = &object, .size = 1},
							 .type_id = type_id_t<editor_project_settings_data_t>::value,
						 });

		void* editor_object = &editor_settings_t::get().configurable;
		_editor_reflection.init(ui,
								_content,
								{
									.fold_states = &_field_states,
									.callbacks =
										{
											.edit_begin		= on_project_settings_edit_begin,
											.edit_submitted = on_project_settings_edit_submitted,
											.user_data		= this,
										},
									.objects = {.data = &editor_object, .size = 1},
									.type_id = type_id_t<editor_settings_configurable_t>::value,
								});

		_command_listener = editor_command_system_t::get().add_listener(on_command_system_event, this);
	}

	void editor_panel_project_settings_t::uninit()
	{
		editor_command_system_t::get().remove_listener(_command_listener);
		_ui->cancel_mutations(this);
		_editor_reflection.uninit();
		_reflection.uninit();
		_scrollbar.uninit();
		_ui->deallocate_widget(_content);
		_ui->deallocate_widget(_scroll_area);
		_field_states.clear();
		_project_edit_previous = {};
		_command_listener	   = {};
		_scroll_area		   = NULL_WIDGET;
		_content			   = NULL_WIDGET;
		_refresh_pending	   = false;
		_project_edit_active   = false;
		editor_panel_t::uninit();
	}

	void editor_panel_project_settings_t::refresh_reflection()
	{
		if (!can_mutate_ui_topology())
		{
			request_refresh_reflection();
			return;
		}

		void* object		= &editor_project_t::get().settings;
		void* editor_object = &editor_settings_t::get().configurable;
		_reflection.save_fold_states();
		_editor_reflection.save_fold_states();

		_reflection.set_reflection({
			.fold_states = &_field_states,
			.callbacks =
				{
					.edit_begin		= on_project_settings_edit_begin,
					.edit_submitted = on_project_settings_edit_submitted,
					.user_data		= this,
				},
			.objects = {.data = &object, .size = 1},
			.type_id = type_id_t<editor_project_settings_data_t>::value,
		});

		_editor_reflection.set_reflection({
			.fold_states = &_field_states,
			.callbacks =
				{
					.edit_begin		= on_project_settings_edit_begin,
					.edit_submitted = on_project_settings_edit_submitted,
					.user_data		= this,
				},
			.objects = {.data = &editor_object, .size = 1},
			.type_id = type_id_t<editor_settings_configurable_t>::value,
		});
	}

	void editor_panel_project_settings_t::request_refresh_reflection()
	{
		_refresh_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_panel_project_settings_t::flush_pending_ui_mutations()
	{
		if (!_refresh_pending)
			return;

		_refresh_pending = false;
		refresh_reflection();
	}

	bool editor_panel_project_settings_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_panel_project_settings_t::begin_project_settings_edit()
	{
		_project_edit_previous = editor_command_project_settings_t::read();
		_project_edit_active   = true;
	}

	void editor_panel_project_settings_t::submit_project_settings_edit()
	{
		if (!_project_edit_active)
			return;

		const editor_command_project_settings_data_t previous = _project_edit_previous;
		editor_project_t::get().settings.project_settings.normalize(&previous.project.project_settings);
		editor_settings_t::get().configurable.normalize();
		const editor_command_project_settings_data_t post = editor_command_project_settings_t::read();
		_project_edit_active							  = false;
		if (previous == post)
			return;

		editor_command_project_settings_t::apply(previous);
		if (!editor_command_project_settings_t::edit(previous, post))
			editor_command_project_settings_t::apply(post);
	}

	void editor_panel_project_settings_t::on_project_settings_edit_begin(void* user_data)
	{
		static_cast<editor_panel_project_settings_t*>(user_data)->begin_project_settings_edit();
	}

	void editor_panel_project_settings_t::on_project_settings_edit_submitted(void* user_data)
	{
		static_cast<editor_panel_project_settings_t*>(user_data)->submit_project_settings_edit();
	}

	void editor_panel_project_settings_t::on_command_system_event(editor_command_system_t&, const editor_command_t& command, void* user_data)
	{
		if (command.type == editor_command_type_e::project_settings_edit)
			static_cast<editor_panel_project_settings_t*>(user_data)->refresh_reflection();
	}

	void editor_panel_project_settings_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_project_settings_t*>(user_data)->flush_pending_ui_mutations();
	}
}
