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
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/editor_payload_controller.hpp"
#include "editor_selection_controller.hpp"
#include "editor_command_system.hpp"
#include "editor_world_metadata.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_panel_entities_t::editor_panel_entities_t()
	{
		set_type(editor_panel_type_e::entities);
		set_title(editor_panel_type_to_string(editor_panel_type_e::entities));
	}

	void editor_panel_entities_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_entity_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_entity_top_row, "entity_top_row");
		tree.attach(_root, _entity_top_row);

		ui::layout_in_t& top_row_in = tree.in(_entity_top_row);
		top_row_in.flags			= ui::wf_visible;
		top_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_row_in.size_value		= {1.0f, theme.item_area_height};
		top_row_in.flow				= ui::flow_e::row;
		top_row_in.child_spacing	= theme.item_spacing;
		top_row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		u8*							data		  = reinterpret_cast<u8*>(&_search_str);
		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.field						  = {
								  .fields = {.data = &data, .size = 1},
								  .type	  = editor_input_field_field_type_e::string,
		  };
		search_config.callbacks.edited	  = on_search_changed;
		search_config.callbacks.user_data = this;
		_search_input.init(ui, _entity_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value.y	   = 0.5f;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {1.0f, theme.item_height};

		_entity_list_area = ui.allocate_widget();
		ui.set_widget_debug_name(_entity_list_area, "entity_list_area");
		tree.attach(_root, _entity_list_area);

		ui::layout_in_t& list_in = tree.in(_entity_list_area);
		list_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable | ui::wf_scroll_y;
		list_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		list_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		list_in.size_mode_y		 = ui::axis_mode_e::fill;
		list_in.size_value		 = {1.0f, 1.0f};
		list_in.flow			 = ui::flow_e::column;
		list_in.child_spacing	 = 0.0f;
		list_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t list_rect = {};
		list_rect.fill_color_a		  = theme.color_frame;
		list_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_entity_list_area, list_rect);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _entity_list_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);

		ui::listener_bundle_t body_listener = {};
		body_listener.user_data				= this;
		body_listener.on_click				= on_entities_body_clicked;
		body_listener.on_wheel				= on_entities_body_wheel;
		body_listener.on_key				= on_entities_key;
		body_listener.on_focus_gain			= on_entities_focus_gain;
		body_listener.on_focus_lose			= on_entities_focus_lost;
		ui.get_input().set_listener(_entity_list_area, body_listener);

		ui.set_pre_layout_tick(_entity_list_area, on_entity_tree_tick, this);

		editor_world_metadata_t::get().get_outliner_rows().reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_payload_entities.reserve(ENTITIES_INITIAL_ROW_CAPACITY);
		_command_listener	= editor_command_system_t::get().add_listener(on_command_system_event, this);
		_selection_listener = editor_selection_controller_t::get().add_listener(on_selection_changed, this);
		editor_payload_controller_t::get().register_listener(on_payload_drop, nullptr, nullptr, this);
		refresh_entities();
	}

	void editor_panel_entities_t::uninit()
	{
		editor_command_system_t::get().remove_listener(_command_listener);
		editor_selection_controller_t::get().remove_listener(_selection_listener);
		editor_payload_controller_t::get().unregister_listener(this);
		_ui->cancel_mutations(this);
		_search_input.uninit();
		_scrollbar.uninit();
		if (editor_tooltip_controller_t* tooltip_controller = editor_tooltip_controller_t::find(*_ui))
		{
			for (const editor_outliner_row_t& row : editor_world_metadata_t::get().get_outliner_rows())
				tooltip_controller->clear_tooltip(row.disable_button);
		}
		_ui->deallocate_widget(_entity_top_row);
		_ui->deallocate_widget(_entity_list_area);

		editor_world_metadata_t::get().get_outliner_rows().clear();
		_payload_entities.clear();

		_entity_top_row		  = NULL_WIDGET;
		_entity_list_area	  = NULL_WIDGET;
		_command_listener	  = {};
		_selection_listener	  = {};
		_main_world			  = {};
		_action_menu_entity	  = NULL_ENTITY_ID;
		_action_menu_folder	  = {};
		_focused_folder		  = {};
		_edit_folder		  = {};
		_entity_generation	  = 0;
		_visible_entity_count = 0;

		editor_panel_t::uninit();
	}

}
