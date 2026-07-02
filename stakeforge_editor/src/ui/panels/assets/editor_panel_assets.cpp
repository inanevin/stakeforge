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
#include "ui/panels/assets/editor_panel_assets.hpp"
#include "ui/panels/assets/editor_panel_assets_internal.hpp"
#include "editor_project.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
namespace sfg
{
	namespace
	{
		editor_dropdown_item_t ASSETS_ITEM_STYLE_ITEMS[] = {
			{.text = "Grid", .value = ASSETS_ITEM_STYLE_ID_GRID},
			{.text = "List", .value = ASSETS_ITEM_STYLE_ID_LIST},
		};
	}
	editor_panel_assets_t::editor_panel_assets_t()
	{
		set_type(editor_panel_type_e::assets);
		set_title(editor_panel_type_to_string(editor_panel_type_e::assets));
	}

	void editor_panel_assets_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_assets_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane, "assets_left_pane");
		tree.attach(_root, _assets_left_pane);

		ui::layout_in_t& left_pane_in	   = tree.in(_assets_left_pane);
		left_pane_in.flags				   = ui::wf_visible;
		left_pane_in.flow				   = ui::flow_e::column;
		left_pane_in.child_spacing		   = 0.0f;
		left_pane_in.size_mode_x		   = ui::axis_mode_e::parent_relative;
		left_pane_in.size_mode_y		   = ui::axis_mode_e::parent_relative;
		left_pane_in.size_value			   = {_pane_split, 1.0f};
		tree.draw_order(_assets_left_pane) = tree.draw_order_const(_root) + 1;

		_assets_left_pane_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_top_row, "assets_left_pane_top_row");
		tree.attach(_assets_left_pane, _assets_left_pane_top_row);

		ui::layout_in_t& top_row_in = tree.in(_assets_left_pane_top_row);
		top_row_in.flags			= ui::wf_visible;
		top_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		top_row_in.size_value		= {1.0f, theme.item_area_height};
		top_row_in.flow				= ui::flow_e::row;
		top_row_in.child_spacing	= theme.item_spacing;
		top_row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		editor_icon_button_config_t filter_button_config = {};
		filter_button_config.frame_color				 = {0.0f, 0.0f, 0.0f, 0.0f};
		filter_button_config.hover_color				 = theme.color_panel_light;
		filter_button_config.press_color				 = theme.color_frame;
		filter_button_config.frame_toggled_color		 = theme.color_frame;
		filter_button_config.icon						 = ICON_FILTER;
		filter_button_config.toggled_icon				 = ICON_FILTER;
		filter_button_config.icon_color					 = theme.color_text0;
		filter_button_config.disabled_color				 = theme.color_text_disabled;
		filter_button_config.tooltip					 = "Filter";
		filter_button_config.size						 = theme.item_height;
		filter_button_config.icon_size					 = theme.text_default_px_size;
		filter_button_config.on_clicked					 = on_filter_button_pressed;
		filter_button_config.user_data					 = this;
		_filter_button.init(ui, _assets_left_pane_top_row, filter_button_config);

		editor_icon_button_config_t refresh_button_config = filter_button_config;
		refresh_button_config.icon						  = ICON_IMPORT;
		refresh_button_config.toggled_icon				  = ICON_IMPORT;
		refresh_button_config.tooltip					  = "Import";
		refresh_button_config.on_clicked				  = on_import_button_pressed;
		_import_button.init(ui, _assets_left_pane_top_row, refresh_button_config);

		refresh_button_config			   = filter_button_config;
		refresh_button_config.icon		   = ICON_ROTATE;
		refresh_button_config.toggled_icon = ICON_ROTATE;
		refresh_button_config.tooltip	   = "Refresh";
		refresh_button_config.on_clicked   = on_refresh_button_pressed;
		_refresh_button.init(ui, _assets_left_pane_top_row, refresh_button_config);

		u8*							search_field  = reinterpret_cast<u8*>(&_search_str);
		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.field						  = {.type = editor_input_field_field_type_e::string, .fields = {.data = &search_field, .size = 1}};
		search_config.on_data_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _assets_left_pane_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value		   = {1.0f, 0.5f};
		search_in.anchor_x		   = ui::anchor_e::end;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::fixed;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {theme.item_width * 1.5f, theme.item_height};

		_assets_left_pane_body = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_body, "assets_left_pane_body");
		tree.attach(_assets_left_pane, _assets_left_pane_body);

		ui::layout_in_t& left_body_in = tree.in(_assets_left_pane_body);
		left_body_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_focusable | ui::wf_scroll_y;
		left_body_in.child_clip_mode  = ui::clip_mode_e::scissor_rect;
		left_body_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		left_body_in.size_mode_y	  = ui::axis_mode_e::fill;
		left_body_in.size_value		  = {1.0f, 1.0f};
		left_body_in.flow			  = ui::flow_e::column;
		left_body_in.child_spacing	  = 0.0f;
		left_body_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t left_body_rect = {};
		left_body_rect.fill_color_a		   = theme.color_frame;
		left_body_rect.fill_color_b		   = theme.color_frame;
		paint.set_rect(_assets_left_pane_body, left_body_rect);

		editor_scrollbar_config_t left_scrollbar_config = {};
		left_scrollbar_config.target					= _assets_left_pane_body;
		left_scrollbar_config.axes						= editor_scrollbar_axis_y;
		_left_scrollbar.init(ui, left_scrollbar_config);

		ui::listener_bundle_t body_listener = {};
		body_listener.user_data				= this;
		body_listener.on_click				= on_assets_body_clicked;
		body_listener.on_wheel				= on_assets_body_wheel;
		body_listener.on_key				= on_asset_tree_key;
		body_listener.on_focus_gain			= on_assets_focus_gain;
		body_listener.on_focus_lose			= on_assets_focus_lost;
		ui.get_input().set_listener(_assets_left_pane_body, body_listener);

		ui.set_pre_layout_tick(_assets_left_pane_body, on_asset_tree_tick, this);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);
		tree.draw_order(_split_border.get_root()) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * ASSETS_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_assets_body_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane, "assets_body_pane");
		tree.attach(_root, _assets_body_pane);
		tree.draw_order(_assets_body_pane) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& body_pane_in = tree.in(_assets_body_pane);
		body_pane_in.flags			  = ui::wf_visible;
		body_pane_in.flow			  = ui::flow_e::column;
		body_pane_in.child_spacing	  = 0.0f;
		body_pane_in.child_margins	  = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};
		body_pane_in.size_mode_x	  = ui::axis_mode_e::fill;
		body_pane_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		body_pane_in.size_value		  = {1.0f, 1.0f};

		_assets_body_pane_top = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_top, "assets_body_pane_top");
		tree.attach(_assets_body_pane, _assets_body_pane_top);

		ui::layout_in_t& body_top_in = tree.in(_assets_body_pane_top);
		body_top_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		body_top_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		body_top_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_top_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_top_in.size_value		 = {1.0f, 1.0f};
		body_top_in.flow			 = ui::flow_e::column;
		body_top_in.child_spacing	 = theme.item_spacing * 0.5f;
		body_top_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t body_top_rect = {};
		body_top_rect.fill_color_a		  = theme.color_panel;
		body_top_rect.fill_color_b		  = theme.color_panel;
		paint.set_rect(_assets_body_pane_top, body_top_rect);

		_assets_body_pane_divider = editor_dividers_t::add_divider_hor(ui, _assets_body_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_assets_body_pane_divider, "assets_body_pane_divider");

		_assets_body_pane_bottom = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_bottom, "assets_body_pane_bottom");
		tree.attach(_assets_body_pane, _assets_body_pane_bottom);

		ui::layout_in_t& body_bottom_in = tree.in(_assets_body_pane_bottom);
		body_bottom_in.flags			= ui::wf_visible;
		body_bottom_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		body_bottom_in.size_mode_y		= ui::axis_mode_e::fixed;
		body_bottom_in.size_value		= {1.0f, theme.item_area_height};
		body_bottom_in.flow				= ui::flow_e::row;
		body_bottom_in.child_spacing	= theme.item_spacing;
		body_bottom_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::listener_bundle_t body_bottom_listener = {};
		body_bottom_listener.user_data			   = this;
		body_bottom_listener.on_click			   = on_assets_body_clicked;
		ui.get_input().set_listener(_assets_body_pane_bottom, body_bottom_listener);

		_assets_body_pane_path = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_path, "assets_body_pane_path");
		tree.attach(_assets_body_pane_bottom, _assets_body_pane_path);

		ui::layout_in_t& path_in = tree.in(_assets_body_pane_path);
		path_in.flags			 = ui::wf_visible;
		path_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		path_in.pos_value.y		 = 0.5f;
		path_in.anchor_y		 = ui::anchor_e::center;
		path_in.size_mode_x		 = ui::axis_mode_e::fill;
		path_in.size_mode_y		 = ui::axis_mode_e::fixed;
		path_in.size_value		 = {1.0f, theme.text_default_px_size};

		ui.set_widget_text(_assets_body_pane_path, "");
		paint.set_text(_assets_body_pane_path,
					   ui.widget_text(_assets_body_pane_path),
					   ui.widget_text_len(_assets_body_pane_path),
					   {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_assets_body_pane_controls = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_controls, "assets_body_pane_controls");
		tree.attach(_assets_body_pane_bottom, _assets_body_pane_controls);

		ui::layout_in_t& controls_in = tree.in(_assets_body_pane_controls);
		controls_in.flags			 = ui::wf_visible;
		controls_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		controls_in.pos_value.y		 = 0.5f;
		controls_in.anchor_y		 = ui::anchor_e::center;
		controls_in.size_mode_x		 = ui::axis_mode_e::fixed;
		controls_in.size_mode_y		 = ui::axis_mode_e::fixed;
		controls_in.flow			 = ui::flow_e::row;
		controls_in.child_spacing	 = theme.item_spacing;
		controls_in.anchor_x		 = ui::anchor_e::end;
		controls_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		controls_in.pos_value.x		 = 1.0f;
		controls_in.size_value		 = {theme.item_width * 2.0f + theme.item_height * 2.0f + theme.item_spacing * 3.0f, theme.item_height};

		editor_icon_button_config_t show_file_assets_config = filter_button_config;
		show_file_assets_config.icon						= ICON_FILE;
		show_file_assets_config.toggled_icon				= ICON_FILE;
		show_file_assets_config.tooltip						= "Show File Assets";
		show_file_assets_config.toggle_enabled				= true;
		show_file_assets_config.toggled						= _show_file_assets;
		show_file_assets_config.on_clicked					= on_show_file_assets_pressed;
		_show_file_assets_button.init(ui, _assets_body_pane_controls, show_file_assets_config);

		editor_icon_button_config_t asset_favourites_only_config = filter_button_config;
		asset_favourites_only_config.icon						 = ICON_STAR;
		asset_favourites_only_config.toggled_icon				 = ICON_STAR;
		asset_favourites_only_config.icon_color					 = theme.color_accent1;
		asset_favourites_only_config.tooltip					 = "Show Only Favourites";
		asset_favourites_only_config.toggle_enabled				 = true;
		asset_favourites_only_config.toggled					 = _asset_favourites_only;
		asset_favourites_only_config.on_clicked					 = on_asset_favourites_only_pressed;
		_asset_favourites_only_button.init(ui, _assets_body_pane_controls, asset_favourites_only_config);

		u8*							asset_search_field	= reinterpret_cast<u8*>(&_asset_search_str);
		editor_input_field_config_t asset_search_config = {};
		asset_search_config.placeholder					= "Search";
		asset_search_config.field						= {.type = editor_input_field_field_type_e::string, .fields = {.data = &asset_search_field, .size = 1}};
		asset_search_config.on_data_changed				= on_asset_search_changed;
		asset_search_config.user_data					= this;
		_asset_search_input.init(ui, _assets_body_pane_controls, asset_search_config);

		ui::layout_in_t& asset_search_in = tree.in(_asset_search_input.get_root());
		asset_search_in.size_mode_x		 = ui::axis_mode_e::fixed;
		asset_search_in.size_mode_y		 = ui::axis_mode_e::fixed;
		asset_search_in.size_value		 = {theme.item_width, theme.item_height};

		editor_dropdown_config_t item_style_config = {};
		item_style_config.items					   = ASSETS_ITEM_STYLE_ITEMS;
		item_style_config.item_count			   = static_cast<u16>(sizeof(ASSETS_ITEM_STYLE_ITEMS) / sizeof(ASSETS_ITEM_STYLE_ITEMS[0]));
		item_style_config.selected				   = get_selected_item_style;
		item_style_config.pressed				   = on_item_style_pressed;
		item_style_config.user_data				   = this;
		item_style_config.width					   = editor_dropdown_width_e::fixed;
		item_style_config.pos_y					   = editor_dropdown_pos_y_e::center;
		item_style_config.fixed_width			   = theme.item_width;
		_item_style_dropdown.init(ui, _assets_body_pane_controls, item_style_config);

		_assets_body_pane_bottom_divider = editor_dividers_t::add_divider_hor(ui, _assets_body_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_assets_body_pane_bottom_divider, "assets_body_pane_bottom_divider");

		editor_scrollbar_config_t right_scrollbar_config = {};
		right_scrollbar_config.target					 = _assets_body_pane_top;
		right_scrollbar_config.axes						 = editor_scrollbar_axis_y;
		_right_scrollbar.init(ui, right_scrollbar_config);

		ui::listener_bundle_t body_top_listener = {};
		body_top_listener.user_data				= this;
		body_top_listener.on_click				= on_assets_body_clicked;
		body_top_listener.on_wheel				= on_assets_body_wheel;
		body_top_listener.on_key				= on_asset_tree_key;
		body_top_listener.on_focus_gain			= on_assets_focus_gain;
		body_top_listener.on_focus_lose			= on_assets_focus_lost;
		ui.get_input().set_listener(_assets_body_pane_top, body_top_listener);

		ui.set_pre_layout_tick(_assets_body_pane_top, on_asset_grid_tick, this);

		_folder_rows.reserve(ASSETS_INITIAL_ROW_CAPACITY);
		_asset_grid_rows.reserve(32);
		_asset_grid_items.reserve(ASSETS_INITIAL_GRID_ITEM_CAPACITY);
		_expanded_folder_hashes.reserve(256);
		_favourite_folder_hashes.reserve(256);
		_favourite_asset_guids.reserve(256);
		_selected_folder_hashes.reserve(ASSETS_INITIAL_ROW_CAPACITY);
		_selected_asset_nodes.reserve(ASSETS_INITIAL_GRID_ITEM_CAPACITY);
		_payload_folder_nodes.reserve(ASSETS_INITIAL_ROW_CAPACITY);
		_payload_asset_nodes.reserve(ASSETS_INITIAL_GRID_ITEM_CAPACITY);
		_asset_grid_body_size_valid = false;
		_asset_grid_rebuild_pending = false;
		editor_payload_controller_t::get().register_listener(on_payload_drop, on_payload_tick, on_payload_end, this);
		apply_pane_split();
		refresh_folder_rows();
	}

	void editor_panel_assets_t::uninit()
	{
		_ui->cancel_mutations(this);
		editor_payload_controller_t::get().unregister_listener(this);
		_folder_payload_highlight_active = false;
		_search_input.uninit();
		_filter_button.uninit();
		_import_button.uninit();
		_refresh_button.uninit();
		_show_file_assets_button.uninit();
		_asset_favourites_only_button.uninit();
		_asset_search_input.uninit();
		_item_style_dropdown.uninit();
		_split_border.uninit();
		_left_scrollbar.uninit();
		_right_scrollbar.uninit();
		clear_asset_grid();
		_ui->deallocate_widget(_assets_left_pane);
		_ui->deallocate_widget(_assets_body_pane);

		_folder_rows.clear();
		_asset_grid_rows.clear();
		_asset_grid_items.clear();
		_expanded_folder_hashes.clear();
		_favourite_folder_hashes.clear();
		_favourite_asset_guids.clear();
		_selected_folder_hashes.clear();
		_selected_asset_nodes.clear();
		_payload_folder_nodes.clear();
		_payload_asset_nodes.clear();
		clear_pending_import();

		editor_panel_t::uninit();
	}

	void editor_panel_assets_t::serialize(nlohmann::json& j) const
	{
		j						   = nlohmann::json::object();
		j["pane_split"]			   = _pane_split;
		j["favourites_only"]	   = _favourites_only;
		j["show_file_assets"]	   = _show_file_assets;
		j["asset_favourites_only"] = _asset_favourites_only;
		j["search_str"]			   = _search_str;
		j["asset_search_str"]	   = _asset_search_str;
		j["favourites"]			   = _favourite_folder_hashes;
		j["asset_favourites"]	   = _favourite_asset_guids;
		switch (_asset_item_style)
		{
		case asset_item_style_e::grid:
			j["item_style"] = "grid";
			break;
		case asset_item_style_e::list:
			j["item_style"] = "list";
			break;
		}
	}

	void editor_panel_assets_t::deserialize(const nlohmann::json& j)
	{
		_pane_split				  = math::clamp(j.value<f32>("pane_split", _pane_split), ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		_favourites_only		  = j.value<bool>("favourites_only", false);
		_show_file_assets		  = j.value<bool>("show_file_assets", false);
		_asset_favourites_only	  = j.value<bool>("asset_favourites_only", false);
		_search_str				  = j.value<string_t>("search_str", {});
		_asset_search_str		  = j.value<string_t>("asset_search_str", {});
		_favourite_folder_hashes  = j.value<vector_t<u64>>("favourites", {});
		_favourite_asset_guids	  = j.value<vector_t<sid_t>>("asset_favourites", {});
		const string_t item_style = j.value<string_t>("item_style", "grid");
		if (item_style == "list")
			_asset_item_style = asset_item_style_e::list;
		else
			_asset_item_style = asset_item_style_e::grid;
		_search_str_lower = _search_str;
		string_util::to_lower(_search_str_lower);
		_asset_search_str_lower = _asset_search_str;
		string_util::to_lower(_asset_search_str_lower);
	}

	void editor_panel_assets_t::make_visible(bool visible)
	{
		editor_panel_t::make_visible(visible);
		if (visible)
			refresh_folder_rows();
	}

	void editor_panel_assets_t::apply_pane_split()
	{
		_ui->get_tree().in(_assets_left_pane).size_value.x = _pane_split;
	}

}
