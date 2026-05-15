// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_log.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_dividers.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/string.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		const editor_dropdown_item_t LOG_SOURCE_ITEMS[] = {
			{.text = "All", .value = static_cast<u16>(log_source_type_e::all)},
			{.text = "Editor", .value = static_cast<u16>(log_source_type_e::editor)},
			{.text = "Game", .value = static_cast<u16>(log_source_type_e::game)},
		};

		const char* log_source_type_to_string(log_source_type_e type)
		{
			switch (type)
			{
			case log_source_type_e::all:
				return "all";
			case log_source_type_e::editor:
				return "editor";
			case log_source_type_e::game:
				return "game";
			}
			return "all";
		}

		log_source_type_e log_source_type_from_string(const char* value)
		{
			const sid_t id = TO_SID(value);
			if (id == TO_SID("editor"))
				return log_source_type_e::editor;
			if (id == TO_SID("game"))
				return log_source_type_e::game;
			return log_source_type_e::all;
		}
	}

	editor_panel_log_t::editor_panel_log_t()
	{
		set_type(editor_panel_type_e::log);
		set_title(editor_panel_type_to_string(editor_panel_type_e::log));
	}

	void editor_panel_log_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_top_row, "log_top_row");
		tree.attach(_root, _top_row);

		ui::layout_in_t& title_in = tree.in(_top_row);
		title_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		title_in.size_mode_y	  = ui::axis_mode_e::fixed;
		title_in.size_value		  = {1.0f, theme.item_area_height};
		title_in.flow			  = ui::flow_e::row;
		title_in.child_spacing	  = theme.item_spacing;
		title_in.child_margins	  = {0.0f, theme.indent_horizontal, 0.0f, theme.indent_horizontal};

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
		button_config.hover_color				  = theme.color_panel_light;
		button_config.press_color				  = theme.color_frame;
		button_config.frame_toggled_color		  = theme.color_frame;
		button_config.size						  = theme.item_height;
		button_config.icon_size					  = theme.text_default_px_size;
		button_config.toggle_enabled			  = true;
		button_config.toggled					  = true;
		button_config.on_clicked				  = on_filter_pressed;

		button_config.icon		   = ICON_WARN;
		button_config.toggled_icon = ICON_WARN;
		button_config.icon_color   = theme.color_accent_warn;
		button_config.tooltip	   = "Warnings";
		button_config.user_data	   = &_show_warn;
		_warn_button.init(ui, _top_row, button_config);

		button_config.icon		   = ICON_ERROR;
		button_config.toggled_icon = ICON_ERROR;
		button_config.icon_color   = theme.color_accent_err;
		button_config.tooltip	   = "Errors";
		button_config.user_data	   = &_show_err;
		_err_button.init(ui, _top_row, button_config);

		button_config.icon		   = ICON_INFO;
		button_config.toggled_icon = ICON_INFO;
		button_config.icon_color   = theme.color_text0;
		button_config.tooltip	   = "Info";
		button_config.user_data	   = &_show_info;
		_info_button.init(ui, _top_row, button_config);

		button_config.icon		   = ICON_TRACE;
		button_config.toggled_icon = ICON_TRACE;
		button_config.icon_color   = theme.color_accent1;
		button_config.tooltip	   = "Trace";
		button_config.user_data	   = &_show_trace;
		_trace_button.init(ui, _top_row, button_config);

		// editor_dividers_t::add_divider_hor(ui, _root, theme.border_thickness, theme.color_panel_light, theme.color_panel_light, ui::vg_gradient_e::none);

		_body = ui.allocate_widget();
		ui.set_widget_debug_name(_body, "log_body");
		tree.attach(_root, _body);

		ui::layout_in_t& body_in = tree.in(_body);
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};

		ui::vg_rect_paint_t body_rect = {};
		body_rect.fill_color_a		  = theme.color_frame;
		body_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_body, body_rect);

		// editor_dividers_t::add_divider_hor(ui, _root, theme.border_thickness, theme.color_panel_light, theme.color_panel_light, ui::vg_gradient_e::none);

		_bottom_row = ui.allocate_widget();
		ui.set_widget_debug_name(_bottom_row, "log_bottom_row");
		tree.attach(_root, _bottom_row);

		ui::layout_in_t& bottom_in = tree.in(_bottom_row);
		bottom_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		bottom_in.size_mode_y	   = ui::axis_mode_e::fixed;
		bottom_in.size_value	   = {1.0f, theme.item_area_height};
		bottom_in.flow			   = ui::flow_e::row;
		bottom_in.child_spacing	   = 0.0f;
		bottom_in.child_margins	   = {0.0f, theme.indent_horizontal, 0.0f, theme.indent_horizontal};
	}

	void editor_panel_log_t::uninit()
	{
		_trace_button.uninit();
		_info_button.uninit();
		_err_button.uninit();
		_warn_button.uninit();
		_source_dropdown.uninit();
		editor_panel_t::uninit();

		_top_row	= NULL_WIDGET;
		_body		= NULL_WIDGET;
		_bottom_row = NULL_WIDGET;
	}

	void editor_panel_log_t::serialize(nlohmann::json& j) const
	{
		j				 = nlohmann::json::object();
		j["source_type"] = log_source_type_to_string(_source_type);
	}

	void editor_panel_log_t::deserialize(const nlohmann::json& j)
	{
		const string_t source_type = j.value<string_t>("source_type", "all");
		_source_type			   = log_source_type_from_string(source_type.c_str());
	}

	void editor_panel_log_t::make_visible(bool visible)
	{
		editor_panel_t::make_visible(visible);
		if (!visible)
			_source_dropdown.close();
	}

	u16 editor_panel_log_t::get_selected_source(void* user_data)
	{
		return static_cast<u16>(static_cast<editor_panel_log_t*>(user_data)->_source_type);
	}

	void editor_panel_log_t::on_source_pressed(u16 value, void* user_data)
	{
		static_cast<editor_panel_log_t*>(user_data)->_source_type = static_cast<log_source_type_e>(value);
	}

	void editor_panel_log_t::on_filter_pressed(bool toggled, void* user_data)
	{
		*static_cast<bool*>(user_data) = toggled;
	}
}
