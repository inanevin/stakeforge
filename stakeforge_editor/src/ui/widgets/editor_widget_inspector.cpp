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
#include "ui/widgets/editor_widget_inspector.hpp"
#include "world_edit/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui	  = &ui;
		_root = parent;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		_scroll_area = ui.allocate_widget();
		ui.set_widget_debug_name(_scroll_area, "inspector_scroll_area");
		tree.attach(_root, _scroll_area);

		ui::layout_in_t& scroll_area_in = tree.in(_scroll_area);
		scroll_area_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		scroll_area_in.child_clip_mode	= ui::clip_mode_e::scissor_rect;
		scroll_area_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_value		= {1.0f, 1.0f};

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "inspector_column");
		tree.attach(_scroll_area, _column);

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.flags			   = ui::wf_visible;
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _scroll_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);
		_command_listener = editor_command_system_t::get().add_listener(on_command_system_event, this);
	}

	void editor_widget_inspector_t::uninit()
	{
		editor_command_system_t::get().remove_listener(_command_listener);
		if (!_selection_listener.is_null())
			editor_world_controller_t::get().get_edit_context(_edit_context).remove_selection_listener(_selection_listener);
		_ui->cancel_mutations(this);
		clear_display();
		_scrollbar.uninit();
		_component_states.clear();
		_field_states.clear();
		_entity_scroll_states.clear();
		_display_entities.clear();
		_display_world		  = nullptr;
		_display_world_handle = {};
		_display_type		  = editor_inspector_display_type_e::none;
		_column				  = NULL_WIDGET;
		_scroll_area		  = NULL_WIDGET;
		_copied_component_stream.destroy();
		_copied_entity_info		   = {};
		_command_listener		   = {};
		_selection_listener		   = {};
		_edit_context			   = {};
		_copied_component_type	   = 0;
		_action_menu_type_id	   = 0;
		_pending_component_type	   = 0;
		_pending_scroll_y		   = 0.0f;
		_refresh_component_pending = false;
		_scroll_restore_pending	   = false;
		_skip_scroll_state_save	   = false;
		_copied_entity_info_valid  = false;

		_root = NULL_WIDGET;
		_ui	  = nullptr;
	}

	void editor_widget_inspector_t::set_edit_context(editor_world_edit_context_handle_t context)
	{
		if (_edit_context == context)
			return;

		if (!_selection_listener.is_null())
		{
			editor_world_controller_t::get().get_edit_context(_edit_context).remove_selection_listener(_selection_listener);
			_selection_listener = {};
		}

		_edit_context = context;
		if (!_edit_context.is_null() && _ui != nullptr)
			_selection_listener = editor_world_controller_t::get().get_edit_context(_edit_context).add_selection_listener(on_selection_changed, this);
	}

}
