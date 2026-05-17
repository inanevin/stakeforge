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
#include "ui/panels/editor_panel_assets.hpp"
#include "editor_app.hpp"
#include "editor_settings.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ASSETS_PANE_SPLIT_MIN			   0.15f
#define ASSETS_PANE_SPLIT_MAX			   0.35f
#define ASSETS_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define ASSETS_FOLDER_INDENT_MULT		   2.0f
#define ASSETS_SCROLL_WHEEL_STEP		   32.0f

	namespace
	{
		enum assets_action_menu_command_e : u16
		{
			assets_action_menu_create_folder	= 1,
			assets_action_menu_delete			= 2,
			assets_action_menu_duplicate		= 3,
			assets_action_menu_toggle_favourite = 4,
			assets_action_menu_open_in_os		= 5,
		};

		const char* assets_filter_to_text(u8 filter)
		{
			return filter == assets_filter_favourites ? "Favourites" : "All";
		}

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			tree.in(id).flags = visible ? static_cast<u16>(ui::wf_visible | (input ? ui::wf_input : 0)) : 0;
		}
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

		ui::layout_in_t& left_pane_in = tree.in(_assets_left_pane);
		left_pane_in.flow			  = ui::flow_e::column;
		left_pane_in.child_spacing	  = 0.0f;
		left_pane_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

		_assets_left_pane_top_row = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_top_row, "assets_left_pane_top_row");
		tree.attach(_assets_left_pane, _assets_left_pane_top_row);

		ui::layout_in_t& top_row_in = tree.in(_assets_left_pane_top_row);
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
		filter_button_config.tooltip					 = "Filter";
		filter_button_config.size						 = theme.item_height;
		filter_button_config.icon_size					 = theme.text_default_px_size;
		filter_button_config.on_clicked					 = on_filter_button_pressed;
		filter_button_config.user_data					 = this;
		_filter_button.init(ui, _assets_left_pane_top_row, filter_button_config);

		editor_icon_button_config_t refresh_button_config = {};
		refresh_button_config.frame_color				  = {0.0f, 0.0f, 0.0f, 0.0f};
		refresh_button_config.hover_color				  = theme.color_panel_light;
		refresh_button_config.press_color				  = theme.color_frame;
		refresh_button_config.frame_toggled_color		  = theme.color_frame;
		refresh_button_config.icon						  = ICON_ROTATE;
		refresh_button_config.toggled_icon				  = ICON_ROTATE;
		refresh_button_config.icon_color				  = theme.color_text0;
		refresh_button_config.tooltip					  = "Refresh";
		refresh_button_config.size						  = theme.item_height;
		refresh_button_config.icon_size					  = theme.text_default_px_size;
		refresh_button_config.on_clicked				  = on_refresh_button_pressed;
		refresh_button_config.user_data					  = this;
		_refresh_button.init(ui, _assets_left_pane_top_row, refresh_button_config);

		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.text_value				  = _search_str.c_str();
		search_config.type						  = editor_input_field_type_e::text;
		search_config.on_text_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _assets_left_pane_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.flags |= ui::wf_overlay;
		search_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value	  = {1.0f, 0.5f};
		search_in.anchor_x	  = ui::anchor_e::end;
		search_in.anchor_y	  = ui::anchor_e::center;
		search_in.size_mode_x = ui::axis_mode_e::fixed;
		search_in.size_mode_y = ui::axis_mode_e::fixed;
		search_in.size_value  = {theme.item_width * 1.5f, theme.item_height};

		_assets_left_pane_body = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_left_pane_body, "assets_left_pane_body");
		tree.attach(_assets_left_pane, _assets_left_pane_body);

		ui::layout_in_t& left_body_in = tree.in(_assets_left_pane_body);
		left_body_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_clip_children | ui::wf_scroll_y;
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
		ui.get_input().set_listener(_assets_left_pane_body, body_listener);

		ui.set_pre_layout_tick(_assets_left_pane_body, on_asset_tree_tick, this);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);

		_assets_body_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane, "assets_body_pane");
		tree.attach(_root, _assets_body_pane);

		ui::layout_in_t& body_pane_in = tree.in(_assets_body_pane);
		body_pane_in.flow			  = ui::flow_e::column;
		body_pane_in.child_spacing	  = 0.0f;
		body_pane_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

		_assets_body_pane_top = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_top, "assets_body_pane_top");
		tree.attach(_assets_body_pane, _assets_body_pane_top);

		ui::layout_in_t& body_top_in = tree.in(_assets_body_pane_top);
		body_top_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_top_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_top_in.size_value		 = {1.0f, 1.0f};

		_assets_body_pane_divider = editor_dividers_t::add_divider_hor(ui, _assets_body_pane, theme.divider_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_assets_body_pane_divider, "assets_body_pane_divider");

		_assets_body_pane_bottom = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_bottom, "assets_body_pane_bottom");
		tree.attach(_assets_body_pane, _assets_body_pane_bottom);

		ui::layout_in_t& body_bottom_in = tree.in(_assets_body_pane_bottom);
		body_bottom_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		body_bottom_in.size_mode_y		= ui::axis_mode_e::fixed;
		body_bottom_in.size_value		= {1.0f, theme.item_area_height};
		body_bottom_in.flow				= ui::flow_e::row;
		body_bottom_in.child_spacing	= theme.item_spacing;
		body_bottom_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		editor_slider_config_t slider_config = {};
		slider_config.label					 = "Size";
		slider_config.on_changed			 = on_thumbnail_slider_changed;
		slider_config.user_data				 = this;
		slider_config.value					 = _thumbnail_slider_value;
		slider_config.min_value				 = 0.2f;
		slider_config.max_value				 = 1.0f;
		slider_config.width					 = theme.item_width * 2.0f;
		slider_config.decimal_count			 = 2;
		slider_config.fixed_width			 = true;
		slider_config.display_label			 = true;
		_thumbnail_slider.init(ui, _assets_body_pane_bottom, slider_config);

		apply_pane_split();
		refresh_folder_rows();
	}

	void editor_panel_assets_t::uninit()
	{
		editor_popup_controller_t::find(*_ui)->close_popup();
		_search_input.uninit();
		_filter_button.uninit();
		_refresh_button.uninit();
		_thumbnail_slider.uninit();
		_split_border.uninit();
		_left_scrollbar.uninit();
		_ui->deallocate_widget(_assets_left_pane_top_row);
		_ui->deallocate_widget(_assets_left_pane_body);
		_ui->deallocate_widget(_assets_left_pane);
		_ui->deallocate_widget(_assets_body_pane_top);
		_ui->deallocate_widget(_assets_body_pane_divider);
		_ui->deallocate_widget(_assets_body_pane_bottom);
		_ui->deallocate_widget(_assets_body_pane);

		_assets_left_pane		  = NULL_WIDGET;
		_assets_left_pane_top_row = NULL_WIDGET;
		_assets_left_pane_body	  = NULL_WIDGET;
		_assets_body_pane		  = NULL_WIDGET;
		_assets_body_pane_top	  = NULL_WIDGET;
		_assets_body_pane_divider = NULL_WIDGET;
		_assets_body_pane_bottom  = NULL_WIDGET;
		_folder_rows.clear();
		_expanded_folder_hashes.clear();
		_favourite_folder_hashes.clear();
		_search_str.clear();
		_action_menu_folder_hash  = 0;
		_selected_folder_hash	  = 0;
		_asset_tree_generation	  = 0;
		_visible_folder_row_count = 0;
		_thumbnail_slider_value	  = 1.0f;
		_has_action_menu_folder	  = false;
		_has_selected_folder	  = false;

		editor_panel_t::uninit();
	}

	void editor_panel_assets_t::serialize(nlohmann::json& j) const
	{
		j							= nlohmann::json::object();
		j["pane_split"]				= _pane_split;
		j["filter"]					= static_cast<u32>(_filter_flags);
		j["search_str"]				= _search_str;
		j["favourites"]				= _favourite_folder_hashes;
		j["thumbnail_slider_value"] = _thumbnail_slider_value;
	}

	void editor_panel_assets_t::deserialize(const nlohmann::json& j)
	{
		_pane_split				 = math::clamp(j.value<f32>("pane_split", _pane_split), ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		const u8 filter			 = static_cast<u8>(j.value<u32>("filter", static_cast<u32>(assets_filter_all))) & static_cast<u8>(assets_filter_all | assets_filter_favourites);
		_filter_flags			 = (filter & assets_filter_favourites) != 0 ? assets_filter_favourites : assets_filter_all;
		_search_str				 = j.value<string_t>("search_str", {});
		_favourite_folder_hashes = j.value<vector_t<u64>>("favourites", {});
		_thumbnail_slider_value	 = math::clamp(j.value<f32>("thumbnail_slider_value", _thumbnail_slider_value), 0.2f, 1.0f);
	}

	void editor_panel_assets_t::make_visible(bool visible)
	{
		editor_panel_t::make_visible(visible);
		if (!visible)
			editor_popup_controller_t::find(*_ui)->close_popup();
		else
			refresh_folder_rows();
	}

	void editor_panel_assets_t::apply_pane_split()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();
		_pane_split					= math::clamp(_pane_split, ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);

		ui::layout_in_t& left_in = tree.in(_assets_left_pane);
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * ASSETS_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		ui::layout_in_t& body_in = tree.in(_assets_body_pane);
		body_in.size_mode_x		 = ui::axis_mode_e::fill;
		body_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		body_in.size_value		 = {1.0f, 1.0f};
	}

	void editor_panel_assets_t::open_filter_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_popup_item_desc_t items[] = {
			{.text = assets_filter_to_text(assets_filter_all), .id = assets_filter_all, .selected = _filter_flags == assets_filter_all},
			{.text = assets_filter_to_text(assets_filter_favourites), .id = assets_filter_favourites, .selected = _filter_flags == assets_filter_favourites},
		};

		const editor_theme_t&	theme = editor_theme_t::get();
		const ui::layout_out_t& out	  = _ui->get_tree().out(_filter_button.get_root());

		editor_popup_desc_t desc = {};
		desc.items				 = items;
		desc.item_count			 = static_cast<u16>(sizeof(items) / sizeof(items[0]));
		desc.pos				 = {out.pos.x, out.pos.y + out.size.y + theme.item_spacing};
		desc.width				 = theme.item_width;
		desc.pressed			 = on_filter_popup_pressed;
		desc.user_data			 = this;
		popup->request_popup(desc);
	}

	void editor_panel_assets_t::open_action_menu(const vec2f_t& pos, bool folder_context, u64 folder_hash)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_has_action_menu_folder	 = folder_context;
		_action_menu_folder_hash = folder_context ? folder_hash : 0;

		_action_menu_create_rows[0] = {
			.text	 = "Folder",
			.command = assets_action_menu_create_folder,
		};

		_action_menu_rows[0] = {
			.text		 = "Create",
			.children	 = _action_menu_create_rows,
			.child_count = static_cast<u16>(sizeof(_action_menu_create_rows) / sizeof(_action_menu_create_rows[0])),
		};
		_action_menu_rows[1] = {
			.text	  = "Delete",
			.command  = assets_action_menu_delete,
			.disabled = !folder_context,
		};
		_action_menu_rows[2] = {
			.text	  = "Duplicate",
			.command  = assets_action_menu_duplicate,
			.disabled = !folder_context,
		};
		_action_menu_rows[3] = {
			.text			= "Toggle Favourite",
			.icon			= ICON_STAR,
			.icon_color		= editor_theme_t::get().color_accent1,
			.command		= assets_action_menu_toggle_favourite,
			.has_icon_color = true,
			.disabled		= !folder_context,
		};
		_action_menu_rows[4] = {
			.text	 = "Open In OS",
			.command = assets_action_menu_open_in_os,
		};

		const editor_theme_t& theme = editor_theme_t::get();

		editor_action_menu_style_t style = {};
		style.dropdown_color			 = theme.color_frame;
		style.hover_color				 = theme.color_panel_light;
		style.press_color				 = theme.color_light;
		style.text_color				 = theme.color_text0;
		style.shortcut_color			 = theme.color_text2;
		style.disabled_text_color		 = theme.color_text_disabled;
		style.title_color				 = theme.color_text2;
		style.title_line_color			 = theme.color_text2;
		style.icon_color				 = theme.color_text0;
		style.min_width					 = theme.item_width * 1.4f;
		style.row_height				 = theme.item_height;
		style.text_size					 = theme.text_default_px_size;
		style.shortcut_size				 = theme.text_small_title_px_size;
		style.title_size				 = theme.text_small_title_px_size;
		style.title_line_thickness		 = theme.divider_thickness;
		style.icon_size					 = theme.icon_default_px_size;
		style.padding_x					 = theme.margin_horizontal;
		style.padding_y					 = theme.margin_vertical;
		style.shortcut_gap				 = theme.item_spacing * 4.0f;
		style.title_gap					 = theme.item_spacing;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = _action_menu_rows;
		desc.row_count				   = static_cast<u16>(sizeof(_action_menu_rows) / sizeof(_action_menu_rows[0]));
		desc.pos					   = pos;
		desc.style					   = style;
		desc.command_fn				   = on_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_assets_t::refresh_folder_rows()
	{
		const editor_asset_manager_t& asset_manager = editor_app_t::get().get_asset_manager();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		_asset_tree_generation						= asset_manager.get_generation();
		_visible_folder_row_count					= 0;

		if (asset_tree.empty() || asset_manager.get_root_node().is_null() || !asset_tree.is_valid(asset_manager.get_root_node()))
		{
			for (const folder_row_t& row : _folder_rows)
				set_folder_row_visible(row, false);
			return;
		}

		const editor_asset_node_t& root = asset_tree.value(asset_manager.get_root_node());
		frame_string_t<char>	   path = root.name.c_str();
		append_folder_rows(asset_manager.get_root_node(), 0, path);

		for (size_t i = _visible_folder_row_count; i < _folder_rows.size(); ++i)
			set_folder_row_visible(_folder_rows[i], false);
	}

	void editor_panel_assets_t::append_folder_rows(editor_asset_node_handle_t node, u16 depth, const frame_string_t<char>& path)
	{
		const editor_asset_manager_t& asset_manager = editor_app_t::get().get_asset_manager();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		const editor_asset_node_t&	  asset_node	= asset_tree.value(node);
		if (is_folder_hidden(asset_node))
			return;

		if (!_search_str.empty() && !folder_subtree_matches_search(node))
			return;

		const u64  path_hash = hashing_t::hash_fnv_1a64(path.c_str());
		const bool promoted	 = is_folder_promoted(asset_node);
		const bool favourite = is_folder_favourite(path_hash);
		const bool visible	 = !promoted && folder_passes_filter(path_hash);

		bool folded = false;
		if (visible)
		{
			const bool has_children = has_visible_folder_child(node, path);
			folded					= _search_str.empty() && has_children && !is_folder_expanded(path_hash);

			folder_row_t& row = get_or_create_folder_row(_visible_folder_row_count++);
			update_folder_row(row, asset_node.name.c_str(), depth, path_hash, has_children, folded, favourite);
		}

		if (folded)
			return;

		editor_asset_node_handle_t child = asset_tree.first_child(node);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = asset_tree.value(child);
			if (child_node.type == editor_asset_node_type_e::folder)
			{
				frame_string_t<char> child_path = path;
				child_path += "/";
				child_path += child_node.name;
				append_folder_rows(child, static_cast<u16>(depth + (visible ? 1 : 0)), child_path);
			}

			child = asset_tree.next_sibling(child);
		}
	}

	editor_panel_assets_t::folder_row_t& editor_panel_assets_t::get_or_create_folder_row(size_t index)
	{
		if (index < _folder_rows.size())
			return _folder_rows[index];

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		folder_row_t row = {};

		row.root = ui.allocate_widget();
		ui.set_widget_debug_name(row.root, "asset_folder_row");
		tree.attach(_assets_left_pane_body, row.root);
		tree.draw_order(row.root) = tree.draw_order_const(_assets_left_pane_body) + 1;

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.flags			= ui::wf_visible | ui::wf_input;
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_height};
		row_in.flow				= ui::flow_e::row;
		row_in.child_spacing	= theme.item_spacing * 0.5f;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = {0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = {0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		paint.set_rect(row.root, row_rect);
		paint.set_hover_color(row.root, theme.color_panel_light);
		paint.set_press_color(row.root, theme.color_light);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_folder_row_clicked;
		listener.on_double_click	   = on_folder_row_double_clicked;
		ui.get_input().set_listener(row.root, listener);

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "asset_folder_row_icon_wrapper");
		tree.attach(row.root, row.icon);
		tree.draw_order(row.icon) = tree.draw_order_const(row.root) + 1;

		ui::layout_in_t& icon_in = tree.in(row.icon);
		icon_in.flags			 = ui::wf_visible;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value.y		 = 0.5f;
		icon_in.anchor_y		 = ui::anchor_e::center;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};

		ui::listener_bundle_t icon_listener = {};
		icon_listener.user_data				= this;
		icon_listener.on_click				= on_folder_icon_clicked;
		ui.get_input().set_listener(row.icon, icon_listener);

		row.icon_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon_text, "asset_folder_row_icon");
		tree.attach(row.icon, row.icon_text);
		tree.draw_order(row.icon_text) = tree.draw_order_const(row.icon) + 1;

		ui::layout_in_t& icon_text_in = tree.in(row.icon_text);
		icon_text_in.flags			  = ui::wf_visible | ui::wf_overlay;
		icon_text_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_value		  = {0.5f, 0.5f};
		icon_text_in.anchor_x		  = ui::anchor_e::center;
		icon_text_in.anchor_y		  = ui::anchor_e::center;
		icon_text_in.size_mode_x	  = ui::axis_mode_e::fixed;
		icon_text_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.star = ui.allocate_widget();
		ui.set_widget_debug_name(row.star, "asset_folder_row_star_wrapper");
		tree.attach(row.root, row.star);
		tree.draw_order(row.star) = tree.draw_order_const(row.root) + 2;

		ui::layout_in_t& star_in = tree.in(row.star);
		star_in.flags			 = 0;
		star_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_value		 = {1.0f, 0.5f};
		star_in.anchor_x		 = ui::anchor_e::end;
		star_in.anchor_y		 = ui::anchor_e::center;
		star_in.size_mode_x		 = ui::axis_mode_e::fixed;
		star_in.size_mode_y		 = ui::axis_mode_e::fixed;
		star_in.size_value		 = {theme.item_height, theme.item_height};

		row.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.star_text, "asset_folder_row_star");
		tree.attach(row.star, row.star_text);
		tree.draw_order(row.star_text) = tree.draw_order_const(row.star) + 1;

		ui::layout_in_t& star_text_in = tree.in(row.star_text);
		star_text_in.flags			  = 0;
		star_text_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		star_text_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		star_text_in.pos_value		  = {0.5f, 0.5f};
		star_text_in.anchor_x		  = ui::anchor_e::center;
		star_text_in.anchor_y		  = ui::anchor_e::center;
		star_text_in.size_mode_x	  = ui::axis_mode_e::fixed;
		star_text_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "asset_folder_row_label");
		tree.attach(row.root, row.label);
		tree.draw_order(row.label) = tree.draw_order_const(row.root) + 1;

		ui::layout_in_t& label_in = tree.in(row.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;

		_folder_rows.push_back(row);
		return _folder_rows.back();
	}

	void editor_panel_assets_t::update_folder_row(folder_row_t& row, const char* name, u16 depth, u64 path_hash, bool has_children, bool is_folded, bool is_favourite)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.path_hash	 = path_hash;
		row.depth		 = depth;
		row.has_children = has_children;
		row.is_favourite = is_favourite;

		set_folder_row_visible(row, true);
		update_folder_row_background(row);

		ui::layout_in_t& row_in		 = tree.in(row.root);
		row_in.child_margins		 = {0.0f, theme.item_height, 0.0f, theme.margin_horizontal + static_cast<f32>(depth) * theme.indent_horizontal * ASSETS_FOLDER_INDENT_MULT};
		tree.in(row.icon).flags		 = has_children ? static_cast<u16>(ui::wf_visible | ui::wf_input) : static_cast<u16>(ui::wf_visible);
		tree.in(row.icon_text).flags = ui::wf_visible | ui::wf_overlay;

		const char* icon = has_children ? (is_folded ? ICON_DD_RIGHT : ICON_DD_DOWN) : "";
		_ui->set_widget_text(row.icon_text, icon);
		paint.set_text(row.icon_text,
					   _ui->widget_text(row.icon_text),
					   _ui->widget_text_len(row.icon_text),
					   {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.star_text, ICON_STAR);
		paint.set_text(row.star_text,
					   _ui->widget_text(row.star_text),
					   _ui->widget_text_len(row.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.label, name);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_assets_t::update_folder_row_background(const folder_row_t& row)
	{
		const editor_theme_t& theme	   = editor_theme_t::get();
		const bool			  selected = _has_selected_folder && row.path_hash == _selected_folder_hash;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = selected ? theme.color_accent1_dim : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = row_rect.fill_color_a;
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? theme.color_accent1_dim : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? theme.color_accent1_dim : theme.color_light);
	}

	void editor_panel_assets_t::refresh_folder_row_backgrounds()
	{
		for (const folder_row_t& row : _folder_rows)
			update_folder_row_background(row);
	}

	void editor_panel_assets_t::set_folder_row_visible(const folder_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, row.root, visible, true);
		set_widget_visible(tree, row.icon, visible, false);
		tree.in(row.icon_text).flags = visible ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : 0;
		tree.in(row.star).flags		 = visible && row.is_favourite ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : 0;
		tree.in(row.star_text).flags = visible && row.is_favourite ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : 0;
		set_widget_visible(tree, row.label, visible, false);
	}

	void editor_panel_assets_t::select_folder_row(u64 path_hash)
	{
		_selected_folder_hash = path_hash;
		_has_selected_folder  = true;
		refresh_folder_row_backgrounds();
	}

	void editor_panel_assets_t::toggle_folder_fold(u64 path_hash)
	{
		for (auto it = _expanded_folder_hashes.begin(); it != _expanded_folder_hashes.end(); ++it)
		{
			if (*it != path_hash)
				continue;

			_expanded_folder_hashes.erase(it);
			refresh_folder_rows();
			return;
		}

		_expanded_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_folder_favourite(u64 path_hash)
	{
		for (auto it = _favourite_folder_hashes.begin(); it != _favourite_folder_hashes.end(); ++it)
		{
			if (*it != path_hash)
				continue;

			_favourite_folder_hashes.erase(it);
			refresh_folder_rows();
			return;
		}

		_favourite_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	bool editor_panel_assets_t::folder_matches_search(const editor_asset_node_t& node) const
	{
		if (_search_str.empty())
			return true;

		frame_string_t<char> name = node.name.c_str();
		frame_string_t<char> text = _search_str.c_str();
		string_util::to_lower(name);
		string_util::to_lower(text);
		return name.find(text) != frame_string_t<char>::npos;
	}

	bool editor_panel_assets_t::folder_subtree_matches_search(editor_asset_node_handle_t node) const
	{
		const editor_asset_tree_t& tree		  = editor_app_t::get().get_asset_manager().get_asset_tree();
		const editor_asset_node_t& asset_node = tree.value(node);
		if (is_folder_hidden(asset_node))
			return false;

		if (!is_folder_promoted(asset_node) && folder_matches_search(asset_node))
			return true;

		editor_asset_node_handle_t child = tree.first_child(node);
		while (!child.is_null())
		{
			if (tree.value(child).type == editor_asset_node_type_e::folder && folder_subtree_matches_search(child))
				return true;

			child = tree.next_sibling(child);
		}
		return false;
	}

	bool editor_panel_assets_t::is_folder_expanded(u64 path_hash) const
	{
		for (u64 hash : _expanded_folder_hashes)
		{
			if (hash == path_hash)
				return true;
		}
		return false;
	}

	bool editor_panel_assets_t::is_folder_favourite(u64 path_hash) const
	{
		for (u64 hash : _favourite_folder_hashes)
		{
			if (hash == path_hash)
				return true;
		}
		return false;
	}

	bool editor_panel_assets_t::is_folder_hidden(const editor_asset_node_t& node) const
	{
		return !node.name.empty() && node.name[0] == '_';
	}

	bool editor_panel_assets_t::is_folder_promoted(const editor_asset_node_t& node) const
	{
		return node.name == "assets";
	}

	bool editor_panel_assets_t::folder_passes_filter(u64 path_hash) const
	{
		return _filter_flags != assets_filter_favourites || is_folder_favourite(path_hash);
	}

	bool editor_panel_assets_t::folder_subtree_has_visible_row(editor_asset_node_handle_t node, const frame_string_t<char>& path) const
	{
		const editor_asset_tree_t& tree		  = editor_app_t::get().get_asset_manager().get_asset_tree();
		const editor_asset_node_t& asset_node = tree.value(node);
		if (is_folder_hidden(asset_node))
			return false;

		if (!_search_str.empty() && !folder_subtree_matches_search(node))
			return false;

		const u64 path_hash = hashing_t::hash_fnv_1a64(path.c_str());
		if (!is_folder_promoted(asset_node) && folder_passes_filter(path_hash))
			return true;

		editor_asset_node_handle_t child = tree.first_child(node);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = tree.value(child);
			if (child_node.type == editor_asset_node_type_e::folder)
			{
				frame_string_t<char> child_path = path;
				child_path += "/";
				child_path += child_node.name;
				if (folder_subtree_has_visible_row(child, child_path))
					return true;
			}

			child = tree.next_sibling(child);
		}
		return false;
	}

	bool editor_panel_assets_t::has_visible_folder_child(editor_asset_node_handle_t node, const frame_string_t<char>& path) const
	{
		const editor_asset_tree_t& tree	 = editor_app_t::get().get_asset_manager().get_asset_tree();
		editor_asset_node_handle_t child = tree.first_child(node);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = tree.value(child);
			if (child_node.type == editor_asset_node_type_e::folder)
			{
				frame_string_t<char> child_path = path;
				child_path += "/";
				child_path += child_node.name;
				if (folder_subtree_has_visible_row(child, child_path))
					return true;
			}

			child = tree.next_sibling(child);
		}
		return false;
	}

	void editor_panel_assets_t::on_filter_popup_pressed(u16 value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._filter_flags			 = value == assets_filter_favourites ? assets_filter_favourites : assets_filter_all;
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_filter_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.open_filter_popup();
	}

	void editor_panel_assets_t::on_refresh_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (editor_app_t::get().get_asset_manager().rescan(editor_settings_t::get().get_project()))
			panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (command != assets_action_menu_toggle_favourite || !panel._has_action_menu_folder)
			return;

		panel.toggle_folder_favourite(panel._action_menu_folder_hash);
	}

	void editor_panel_assets_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._search_str			 = value != nullptr ? value : "";
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_thumbnail_slider_changed(f32 value, void* user_data)
	{
		editor_panel_assets_t& panel  = *static_cast<editor_panel_assets_t*>(user_data);
		panel._thumbnail_slider_value = value;
	}

	void editor_panel_assets_t::on_assets_body_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.open_action_menu(pos, false, 0);
	}

	void editor_panel_assets_t::on_assets_body_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_panel_assets_t&	panel = *static_cast<editor_panel_assets_t*>(user_data);
		ui::layout_tree_t&		tree  = panel._ui->get_tree();
		ui::layout_in_t&		in	  = tree.in(panel._assets_left_pane_body);
		const ui::layout_out_t& out	  = tree.out(panel._assets_left_pane_body);
		in.scroll_offset.y			  = math::clamp(in.scroll_offset.y + delta * ASSETS_SCROLL_WHEEL_STEP, -out.max_scroll.y, 0.0f);
	}

	void editor_panel_assets_t::on_split_border_drag(editor_split_border_t&, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_panel_assets_t&	assets_panel = *static_cast<editor_panel_assets_t*>(user_data);
		const ui::layout_out_t& out			 = assets_panel._ui->get_tree().out(assets_panel._root);
		SFG_ASSERT(out.size.x > 0.0f);

		assets_panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		assets_panel.apply_pane_split();
	}

	void editor_panel_assets_t::on_asset_tree_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_assets_t&		  panel			= *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_manager_t& asset_manager = editor_app_t::get().get_asset_manager();
		if (panel._asset_tree_generation != asset_manager.get_generation())
			panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_folder_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		for (const folder_row_t& row : panel._folder_rows)
		{
			if (row.icon != id)
				continue;

			if (btn == ui::mouse_button_e::right)
				panel.open_action_menu(pos, true, row.path_hash);
			else if (row.has_children)
				panel.toggle_folder_fold(row.path_hash);
			return;
		}
	}

	void editor_panel_assets_t::on_folder_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		for (const folder_row_t& row : panel._folder_rows)
		{
			if (row.root != id)
				continue;

			if (btn == ui::mouse_button_e::right)
				panel.open_action_menu(pos, true, row.path_hash);
			else
				panel.select_folder_row(row.path_hash);
			return;
		}
	}

	void editor_panel_assets_t::on_folder_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		for (const folder_row_t& row : panel._folder_rows)
		{
			if (row.root != id || !row.has_children)
				continue;

			panel.toggle_folder_fold(row.path_hash);
			return;
		}
	}
}
