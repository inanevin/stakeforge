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
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	editor_panel_log_t::editor_panel_log_t()
	{
		set_type(editor_panel_type_e::log);
		set_title(editor_panel_type_to_string(editor_panel_type_e::log));
		set_icon(ICON_PEN);
	}

	vector_t<editor_panel_log_t::log_record_t> editor_panel_log_t::_stored_logs				 = {};
	vector_t<editor_panel_log_t::log_record_t> editor_panel_log_t::_pending_logs			 = {};
	mutex_t									   editor_panel_log_t::_log_storage_mtx			 = {};
	u64										   editor_panel_log_t::_next_stored_log_sequence = 1;
	u32										   editor_panel_log_t::_log_storage_generation	 = 1;
	bool									   editor_panel_log_t::_log_listener_installed	 = false;

	void editor_panel_log_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		install_log_listener();
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_top_row, "log_top_row");
		tree.attach(_root, _top_row);

		ui::layout_in_t& top_in = tree.in(_top_row);
		top_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_in.size_value		= {1.0f, theme.item_area_height};
		top_in.flow				= ui::flow_e::row;
		top_in.child_spacing	= theme.item_spacing;
		top_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		editor_dropdown_config_t source_dropdown_config = {};
		source_dropdown_config.items					= LOG_SOURCE_ITEMS;
		source_dropdown_config.item_count				= static_cast<u16>(sizeof(LOG_SOURCE_ITEMS) / sizeof(LOG_SOURCE_ITEMS[0]));
		source_dropdown_config.selected					= get_selected_source;
		source_dropdown_config.pressed					= on_source_pressed;
		source_dropdown_config.user_data				= this;
		source_dropdown_config.width					= editor_dropdown_width_e::fixed;
		source_dropdown_config.pos_y					= editor_dropdown_pos_y_e::center;
		source_dropdown_config.fixed_width				= theme.item_width;
		_source_dropdown.init(ui, _top_row, source_dropdown_config);

		editor_icon_button_config_t button_config = {};
		button_config.frame_color				  = {0.0f, 0.0f, 0.0f, 0.0f};
		button_config.outline_color				  = vec4f_t::zero;
		button_config.toggled_frame_color		  = theme.color_frame_light;
		button_config.toggled_outline_color		  = theme.color_outline_light;
		button_config.hover_color				  = theme.color_panel_light;
		button_config.toggled_hover_color		  = theme.color_panel_light;
		button_config.press_color				  = theme.color_frame_light;
		button_config.size						  = theme.item_height;
		button_config.icon_size					  = theme.text_big_px_size;
		button_config.rounding					  = theme.item_rounding;
		button_config.toggle_enabled			  = true;
		button_config.on_clicked				  = on_filter_pressed;

		const struct
		{
			const char* icon;
			vec4f_t		color;
			const char* tooltip;
			u8			flag;
		} filter_specs[FILTER_BUTTON_COUNT] = {
			{ICON_WARN, theme.color_accent_warn, "Warnings", log_level_filter_warn},
			{ICON_ERROR, theme.color_accent_err, "Errors", log_level_filter_err},
			{ICON_INFO, theme.color_text0, "Info", log_level_filter_info},
			{ICON_TRACE, theme.color_accent1, "Trace", log_level_filter_trace},
		};

		for (size_t i = 0; i < FILTER_BUTTON_COUNT; ++i)
		{
			_filter_button_data[i]	   = {.panel = this, .flag = filter_specs[i].flag};
			button_config.icon		   = filter_specs[i].icon;
			button_config.toggled_icon = filter_specs[i].icon;
			button_config.icon_color   = filter_specs[i].color;
			button_config.tooltip	   = filter_specs[i].tooltip;
			button_config.toggled	   = (_log_filter_flags & filter_specs[i].flag) != 0;
			button_config.user_data	   = &_filter_button_data[i];
			_filter_buttons[i].init(ui, _top_row, button_config);
		}

		button_config.icon		   = ICON_DD_DOWN;
		button_config.toggled_icon = ICON_DD_DOWN;
		button_config.icon_color   = theme.color_text0;
		button_config.tooltip	   = "Collapse";
		button_config.toggled	   = _is_collapsed;
		button_config.user_data	   = this;
		button_config.on_clicked   = on_collapse_pressed;
		_collapse_button.init(ui, _top_row, button_config);

		button_config.icon			 = ICON_TRASH;
		button_config.toggled_icon	 = nullptr;
		button_config.icon_color	 = theme.color_accent_err;
		button_config.tooltip		 = "Clear";
		button_config.toggled		 = false;
		button_config.toggle_enabled = false;
		button_config.user_data		 = this;
		button_config.on_clicked	 = on_clear_pressed;
		_clear_button.init(ui, _top_row, button_config);

		u8*							search_field  = reinterpret_cast<u8*>(&_search_text);
		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.field						  = {
			.fields = {.data = &search_field, .size = 1},
			.type	= editor_input_field_field_type_e::string,
		};
		search_config.callbacks.edited	  = on_search_changed;
		search_config.callbacks.user_data = this;
		_search_input.init(ui, _top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value		   = {1.0f, 0.5f};
		search_in.anchor_x		   = ui::anchor_e::end;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::fill;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {1, theme.item_height};

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "log_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_x | ui::wf_scroll_y;
		body_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};
		body_in.flow			 = ui::flow_e::column;
		body_in.child_spacing	 = 0.0f;
		body_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t body_rect = {};
		body_rect.fill_color_a		  = theme.color_frame;
		body_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_body, body_rect);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _body;
		scrollbar_config.axes					   = editor_scrollbar_axis_xy;
		_scrollbar.init(ui, scrollbar_config);

		_rows.reserve(EDITOR_LOG_PANEL_ROW_CAPACITY);
		_drained_logs.reserve(32);
		ui.set_pre_layout_tick(_body, on_log_tick, this);
		drain_pending_logs();
	}

	void editor_panel_log_t::uninit()
	{
		_ui->cancel_mutations(this);
		_scrollbar.uninit();
		for (editor_icon_button_t& button : _filter_buttons)
			button.uninit();
		_collapse_button.uninit();
		_clear_button.uninit();
		_search_input.uninit();
		_source_dropdown.uninit();
		_rows.clear();
		_drained_logs.clear();
		editor_panel_t::uninit();
	}

	void editor_panel_log_t::serialize(nlohmann::json& j) const
	{
		j					  = nlohmann::json::object();
		j["source_type"]	  = log_source_filter_to_string(_source_filter);
		j["log_filter_flags"] = static_cast<u32>(_log_filter_flags);
		j["is_collapsed"]	  = _is_collapsed;
	}

	void editor_panel_log_t::deserialize(const nlohmann::json& j)
	{
		const string_t source_type = j.value<string_t>("source_type", "all");
		_source_filter			   = log_source_filter_from_string(source_type.c_str());
		_log_filter_flags		   = static_cast<u8>(j.value<u32>("log_filter_flags", static_cast<u32>(log_level_filter_all)));
		_is_collapsed			   = j.value("is_collapsed", false);
	}

	void editor_panel_log_t::make_visible(bool visible)
	{
		_is_visible = visible;
		editor_panel_t::make_visible(visible);
		if (visible)
			refresh_log_filter_visibility();
		else
			_source_dropdown.close();
	}

	u16 editor_panel_log_t::get_selected_source(void* user_data)
	{
		return static_cast<u16>(static_cast<editor_panel_log_t*>(user_data)->_source_filter);
	}

}
