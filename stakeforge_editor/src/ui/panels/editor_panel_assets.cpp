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
#include "assets/editor_asset_types.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/frame_string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>

namespace sfg
{
#define ASSETS_PANE_SPLIT_MIN			   0.15f
#define ASSETS_PANE_SPLIT_MAX			   0.35f
#define ASSETS_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define ASSETS_FOLDER_INDENT_MULT		   2.0f
#define ASSETS_SCROLL_WHEEL_STEP		   32.0f
#define ASSETS_INITIAL_ROW_CAPACITY		   64
#define ASSETS_INITIAL_GRID_ITEM_CAPACITY  128
#define ASSETS_FILTER_ID_ALL			   0
#define ASSETS_FILTER_ID_FAVOURITES		   1
#define ASSETS_ITEM_STYLE_ID_GRID		   0
#define ASSETS_ITEM_STYLE_ID_LIST		   1
#define ASSETS_CREATE_DESC_MAX			   255

	namespace
	{
		enum assets_action_menu_command_e : u16
		{
			assets_action_menu_create_folder				  = 1,
			assets_action_menu_delete						  = 2,
			assets_action_menu_duplicate					  = 3,
			assets_action_menu_toggle_favourite				  = 4,
			assets_action_menu_open_directory				  = 5,
			assets_action_menu_rename						  = 6,
			assets_action_menu_create_animation_state_machine = 7,
			assets_action_menu_create_opaque_shader			  = 8,
			assets_action_menu_create_transparent_shader	  = 9,
			assets_action_menu_create_post_process_shader	  = 10,
			assets_action_menu_create_ui_shader				  = 11,
			assets_action_menu_create_ui_text_shader		  = 12,
			assets_action_menu_create_texture_sampler		  = 13,
			assets_action_menu_create_gbuffer_material		  = 14,
			assets_action_menu_create_forward_material		  = 15,
			assets_action_menu_create_physical_material		  = 16,
			assets_item_action_menu_rename					  = 17,
			assets_item_action_menu_duplicate				  = 18,
			assets_item_action_menu_delete					  = 19,
			assets_item_action_menu_open_directory			  = 20,
			assets_item_action_menu_toggle_favourite		  = 21,
		};

		struct create_asset_command_t
		{
			editor_asset_type_e asset_type = editor_asset_type_e::invalid;
			u8					sub_type   = 0;
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_ANIMATION_ROWS[] = {
			{.text = "Animation State Machine", .command = assets_action_menu_create_animation_state_machine},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GRAPHICS_ROWS[] = {
			{.text = "Opaque Shader", .command = assets_action_menu_create_opaque_shader},
			{.text = "Transparent Shader", .command = assets_action_menu_create_transparent_shader},
			{.text = "Post Process Shader", .command = assets_action_menu_create_post_process_shader},
			{.text = "UI Shader", .command = assets_action_menu_create_ui_shader},
			{.text = "UI Text Shader", .command = assets_action_menu_create_ui_text_shader},
			{.text = "Texture Sampler", .command = assets_action_menu_create_texture_sampler},
			{.text = "GBuffer Material", .command = assets_action_menu_create_gbuffer_material},
			{.text = "Forward Material", .command = assets_action_menu_create_forward_material},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GAMEPLAY_ROWS[] = {
			{.text = "C# Script"},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_PHYSICS_ROWS[] = {
			{.text = "Physical Material", .command = assets_action_menu_create_physical_material},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_CREATE_ROWS[] = {
			{.text = "Folder", .command = assets_action_menu_create_folder},
			{.text = "Animation", .children = ASSETS_ACTION_MENU_ANIMATION_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_ANIMATION_ROWS) / sizeof(ASSETS_ACTION_MENU_ANIMATION_ROWS[0]))},
			{.text = "Graphics", .children = ASSETS_ACTION_MENU_GRAPHICS_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_GRAPHICS_ROWS) / sizeof(ASSETS_ACTION_MENU_GRAPHICS_ROWS[0]))},
			{.text = "Gameplay", .children = ASSETS_ACTION_MENU_GAMEPLAY_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_GAMEPLAY_ROWS) / sizeof(ASSETS_ACTION_MENU_GAMEPLAY_ROWS[0]))},
			{.text = "Physics", .children = ASSETS_ACTION_MENU_PHYSICS_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_PHYSICS_ROWS) / sizeof(ASSETS_ACTION_MENU_PHYSICS_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ASSETS_ACTION_MENU_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_CREATE_ROWS) / sizeof(ASSETS_ACTION_MENU_CREATE_ROWS[0]))},
			{.text = "Import"},
			{.text = "Delete", .command = assets_action_menu_delete},
			{.text = "Duplicate", .command = assets_action_menu_duplicate},
			{.text = "Rename", .command = assets_action_menu_rename},
			{.text = "Toggle Favourite", .icon = ICON_STAR, .command = assets_action_menu_toggle_favourite, .has_icon_color = true},
			{.text = "Open In OS", .command = assets_action_menu_open_directory},
		};

		editor_action_menu_row_desc_t ASSETS_ITEM_ACTION_MENU_ROWS[] = {
			{.text = "Rename", .command = assets_item_action_menu_rename},
			{.text = "Duplicate", .command = assets_item_action_menu_duplicate},
			{.text = "Delete", .command = assets_item_action_menu_delete},
			{.text = "Open in OS", .command = assets_item_action_menu_open_directory},
			{.text = "Toggle Favourite", .icon = ICON_STAR, .command = assets_item_action_menu_toggle_favourite, .has_icon_color = true},
		};

		editor_dropdown_item_t ASSETS_ITEM_STYLE_ITEMS[] = {
			{.text = "Grid", .value = ASSETS_ITEM_STYLE_ID_GRID},
			{.text = "List", .value = ASSETS_ITEM_STYLE_ID_LIST},
		};

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			u16 flags = 0;
			if (visible)
			{
				flags = ui::wf_visible;
				if (input)
					flags |= ui::wf_input;
			}
			tree.in(id).flags = flags;
		}

		create_asset_command_t asset_type_from_create_command(u16 command)
		{
			switch (command)
			{
			case assets_action_menu_create_animation_state_machine:
				return {.asset_type = editor_asset_type_e::animation_state_machine};
			case assets_action_menu_create_opaque_shader:
				return {.asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(shader_type_e::opaque_shader)};
			case assets_action_menu_create_transparent_shader:
				return {.asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(shader_type_e::transparent_shader)};
			case assets_action_menu_create_post_process_shader:
				return {.asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(shader_type_e::post_process_shader)};
			case assets_action_menu_create_ui_shader:
				return {.asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(shader_type_e::ui_shader)};
			case assets_action_menu_create_ui_text_shader:
				return {.asset_type = editor_asset_type_e::shader, .sub_type = static_cast<u8>(shader_type_e::ui_text_shader)};
			case assets_action_menu_create_texture_sampler:
				return {.asset_type = editor_asset_type_e::texture_sampler};
			case assets_action_menu_create_gbuffer_material:
				return {.asset_type = editor_asset_type_e::material, .sub_type = static_cast<u8>(editor_material_type_e::gbuffer)};
			case assets_action_menu_create_forward_material:
				return {.asset_type = editor_asset_type_e::material, .sub_type = static_cast<u8>(editor_material_type_e::forward)};
			case assets_action_menu_create_physical_material:
				return {.asset_type = editor_asset_type_e::physical_material};
			default:
				return {};
			}
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
		left_pane_in.flags			  = ui::wf_visible;
		left_pane_in.flow			  = ui::flow_e::column;
		left_pane_in.child_spacing	  = 0.0f;
		left_pane_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		left_pane_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		left_pane_in.size_value		  = {_pane_split, 1.0f};

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

		editor_input_field_config_t search_config = {};
		search_config.placeholder				  = "Search";
		search_config.text_value				  = _search_str.c_str();
		search_config.type						  = editor_input_field_type_e::text;
		search_config.on_text_changed			  = on_search_changed;
		search_config.user_data					  = this;
		_search_input.init(ui, _assets_left_pane_top_row, search_config);

		ui::layout_in_t& search_in = tree.in(_search_input.get_root());
		search_in.flags |= ui::wf_visible | ui::wf_overlay;
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
		left_body_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
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
		ui.get_input().set_listener(_assets_left_pane_body, body_listener);

		ui.set_pre_layout_tick(_assets_left_pane_body, on_asset_tree_tick, this);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * ASSETS_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_assets_body_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane, "assets_body_pane");
		tree.attach(_root, _assets_body_pane);

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
		body_top_in.flags			 = ui::wf_visible | ui::wf_scroll_y;
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

		_assets_body_pane_divider = editor_dividers_t::add_divider_hor(ui, _assets_body_pane, theme.divider_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
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
		tree.draw_order(_assets_body_pane_path) = tree.draw_order_const(_assets_body_pane_bottom);

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
		tree.draw_order(_assets_body_pane_controls) = tree.draw_order_const(_assets_body_pane_bottom);

		ui::layout_in_t& controls_in = tree.in(_assets_body_pane_controls);
		controls_in.flags			 = ui::wf_visible;
		controls_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		controls_in.pos_value.y		 = 0.5f;
		controls_in.anchor_y		 = ui::anchor_e::center;
		controls_in.size_mode_x		 = ui::axis_mode_e::fixed;
		controls_in.size_mode_y		 = ui::axis_mode_e::fixed;
		controls_in.size_value		 = {theme.item_width * 2.0f + theme.item_spacing, theme.item_height};
		controls_in.flow			 = ui::flow_e::row;
		controls_in.child_spacing	 = theme.item_spacing;
		controls_in.anchor_x		 = ui::anchor_e::end;
		controls_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		controls_in.pos_value.x		 = 1.0f;

		editor_input_field_config_t asset_search_config = {};
		asset_search_config.placeholder					= "Search";
		asset_search_config.text_value					= _asset_search_str.c_str();
		asset_search_config.type						= editor_input_field_type_e::text;
		asset_search_config.on_text_changed				= on_asset_search_changed;
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

		_assets_body_pane_bottom_divider = editor_dividers_t::add_divider_hor(ui, _assets_body_pane, theme.divider_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_assets_body_pane_bottom_divider, "assets_body_pane_bottom_divider");

		editor_scrollbar_config_t right_scrollbar_config = {};
		right_scrollbar_config.target					 = _assets_body_pane_top;
		right_scrollbar_config.axes						 = editor_scrollbar_axis_y;
		_right_scrollbar.init(ui, right_scrollbar_config);

		ui::listener_bundle_t body_top_listener = {};
		body_top_listener.user_data				= this;
		body_top_listener.on_click				= on_assets_body_clicked;
		body_top_listener.on_wheel				= on_assets_body_wheel;
		ui.get_input().set_listener(_assets_body_pane_top, body_top_listener);

		ui.set_pre_layout_tick(_assets_body_pane_top, on_asset_grid_tick, this);

		_folder_rows.reserve(ASSETS_INITIAL_ROW_CAPACITY);
		_asset_grid_rows.reserve(32);
		_asset_grid_items.reserve(ASSETS_INITIAL_GRID_ITEM_CAPACITY);
		_expanded_folder_hashes.reserve(256);
		_favourite_folder_hashes.reserve(256);
		_favourite_asset_guids.reserve(256);
		apply_pane_split();
		refresh_folder_rows();
	}

	void editor_panel_assets_t::uninit()
	{
		_search_input.uninit();
		_filter_button.uninit();
		_import_button.uninit();
		_refresh_button.uninit();
		_asset_search_input.uninit();
		_item_style_dropdown.uninit();
		_split_border.uninit();
		_left_scrollbar.uninit();
		_right_scrollbar.uninit();
		clear_asset_grid();
		_ui->deallocate_widget(_assets_left_pane_top_row);
		_ui->deallocate_widget(_assets_left_pane_body);
		_ui->deallocate_widget(_assets_left_pane);
		_ui->deallocate_widget(_assets_body_pane_top);
		_ui->deallocate_widget(_assets_body_pane_divider);
		_ui->deallocate_widget(_assets_body_pane_path);
		_ui->deallocate_widget(_assets_body_pane_controls);
		_ui->deallocate_widget(_assets_body_pane_bottom);
		_ui->deallocate_widget(_assets_body_pane_bottom_divider);
		_ui->deallocate_widget(_assets_body_pane);

		_folder_rows.clear();
		_asset_grid_rows.clear();
		_asset_grid_items.clear();
		_expanded_folder_hashes.clear();
		_favourite_folder_hashes.clear();
		_favourite_asset_guids.clear();
		clear_pending_create_assets();

		editor_panel_t::uninit();
	}

	void editor_panel_assets_t::serialize(nlohmann::json& j) const
	{
		j					  = nlohmann::json::object();
		j["pane_split"]		  = _pane_split;
		j["favourites_only"]  = _favourites_only;
		j["search_str"]		  = _search_str;
		j["asset_search_str"] = _asset_search_str;
		j["favourites"]		  = _favourite_folder_hashes;
		j["asset_favourites"] = _favourite_asset_guids;
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

	void editor_panel_assets_t::open_filter_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_popup_item_desc_t items[] = {
			{.text = "All", .id = ASSETS_FILTER_ID_ALL, .selected = !_favourites_only},
			{.text = "Favourites", .id = ASSETS_FILTER_ID_FAVOURITES, .selected = _favourites_only},
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

	void editor_panel_assets_t::open_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_pos		  = pos;
		const bool folder_context = find_row_by_hash(_selected_folder_hash) != nullptr;

		ASSETS_ACTION_MENU_ROWS[2].disabled	  = !folder_context;
		ASSETS_ACTION_MENU_ROWS[3].disabled	  = !folder_context;
		ASSETS_ACTION_MENU_ROWS[4].disabled	  = !folder_context;
		ASSETS_ACTION_MENU_ROWS[5].disabled	  = !folder_context;
		ASSETS_ACTION_MENU_ROWS[5].icon_color = editor_theme_t::get().color_accent1;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ASSETS_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_ROWS) / sizeof(ASSETS_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_action_menu_command;
		desc.command_user_data		   = this;
		desc.closed_fn				   = on_action_menu_closed;
		desc.closed_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_assets_t::open_asset_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		ASSETS_ITEM_ACTION_MENU_ROWS[4].icon_color = editor_theme_t::get().color_accent1;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ASSETS_ITEM_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ASSETS_ITEM_ACTION_MENU_ROWS) / sizeof(ASSETS_ITEM_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_asset_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_panel_assets_t::import_assets(const vector_t<string_t>& paths)
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();

		frame_vector_t<editor_asset_create_desc_t> create_descs;
		create_descs.reserve(paths.size());

		for (const string_t& path : paths)
		{
			if (create_descs.size() == ASSETS_CREATE_DESC_MAX)
				break;

			const string_t					 extension	= file_system_t::get_file_extension(path);
			const editor_asset_descriptor_t* descriptor = asset_manager.find_asset_descriptor(extension);
			if (descriptor == nullptr)
				continue;

			create_descs.push_back({
				.source_full_path = file_system_t::get_absolute_path(path.c_str()),
				.name			  = file_system_t::get_filename_from_path(path),
				.asset_type		  = descriptor->asset_type,
			});
		}

		if (create_descs.empty())
			return;

		const string_t target_path = get_action_menu_target_folder_path();
		const string_t parent_path = editor_asset_util_t::normalize_directory(target_path.c_str());
		if (parent_path.empty())
			return;

		frame_vector_t<frame_string_t<char>> overwrite_paths;
		overwrite_paths.reserve(create_descs.size());
		for (editor_asset_create_desc_t& desc : create_descs)
		{
			const string_t& asset_name = desc.name;
			if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
				continue;

			frame_string_t<char> asset_path;
			asset_path.assign(parent_path.c_str(), parent_path.size());
			asset_path.append(asset_name.c_str(), asset_name.size());
			asset_path += ".sfg_asset";
			if (file_system_t::exists(asset_path.c_str()))
			{
				desc.guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
				overwrite_paths.push_back(asset_path);
			}
		}

		if (overwrite_paths.empty())
		{
			request_create_assets(parent_path.c_str(), create_descs.data(), static_cast<u8>(create_descs.size()), false);
			return;
		}

		_pending_import_directory.assign(parent_path.c_str(), parent_path.size());
		_pending_import_create_descs.resize(0);
		_pending_import_create_descs.reserve(create_descs.size());
		for (const editor_asset_create_desc_t& desc : create_descs)
			_pending_import_create_descs.push_back(desc);

		frame_vector_t<const char*> row_texts;
		row_texts.reserve(overwrite_paths.size());
		for (size_t i = 0; i < overwrite_paths.size(); ++i)
			row_texts.push_back(overwrite_paths[i].c_str());
		_assets_override_modal.set_rows(row_texts.data(), static_cast<u16>(row_texts.size()));

		editor_modal_button_desc_t buttons[] = {
			{.text = "YES", .callback = on_import_overwrite_confirmed, .user_data = this},
			{.text = "NO", .callback = on_import_overwrite_cancelled, .user_data = this},
		};

		editor_modal_controller_t* modal = editor_modal_controller_t::find(*_ui);
		SFG_ASSERT(modal != nullptr);
		editor_modal_content_desc_t override_content = _assets_override_modal.get_content_desc();
		modal->request_modal("Import Assets", "These assets will be overridden with your import. Continue?", true, buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), &override_content, editor_modal_severity_e::warning);
	}

	void editor_panel_assets_t::refresh_folder_rows()
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		_asset_tree_generation						= asset_manager.get_generation();
		_visible_folder_row_count					= 0;

		const editor_asset_node_handle_t root_handle = asset_manager.get_root_node();
		if (!asset_tree.empty() && !root_handle.is_null() && asset_tree.is_valid(root_handle))
		{
			const editor_asset_node_t& root = asset_tree.value(root_handle);
			frame_string_t<char>	   current_path;
			current_path.assign(root.name.c_str(), root.name.size());
			append_folder_rows(root_handle, 0, current_path);
		}

		for (size_t i = _visible_folder_row_count; i < _folder_rows.size(); ++i)
			set_folder_row_visible(_folder_rows[i], false);

		update_current_directory_label();
		refresh_asset_grid(true);
	}

	bool editor_panel_assets_t::append_folder_rows(editor_asset_node_handle_t node, u16 depth, frame_string_t<char>& current_path)
	{
		const editor_asset_tree_t& tree		  = editor_asset_manager_t::get().get_asset_tree();
		const editor_asset_node_t& asset_node = tree.value(node);
		if ((asset_node.flags & editor_asset_node_flag_hidden) != 0)
			return false;

		const u64  path_hash	 = hashing_t::hash_u64(current_path.c_str(), current_path.size());
		const bool search_active = !_search_str_lower.empty();
		const bool promoted		 = (asset_node.flags & editor_asset_node_flag_promoted) != 0;
		const bool favourite	 = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), path_hash) != _favourite_folder_hashes.end();
		const bool passes_filter = !_favourites_only || favourite || has_favourite_asset_descendant(node);

		bool self_matches_search = !search_active;
		if (search_active)
		{
			frame_string_t<char> name_lower;
			name_lower.assign(asset_node.name.c_str(), asset_node.name.size());
			string_util::to_lower(name_lower);
			self_matches_search = name_lower.find(_search_str_lower.c_str()) != frame_string_t<char>::npos;
		}

		const bool is_expanded	 = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), path_hash) != _expanded_folder_hashes.end();
		const bool may_emit_self = !promoted && passes_filter;
		const bool will_fold	 = may_emit_self && !search_active && !is_expanded;

		const size_t my_slot	 = may_emit_self ? _visible_folder_row_count++ : 0;
		const u16	 child_depth = static_cast<u16>(depth + (may_emit_self ? 1 : 0));
		bool		 any_visible = false;

		if (!will_fold)
		{
			editor_asset_node_handle_t child = tree.first_child(node);
			while (!child.is_null())
			{
				const editor_asset_node_t& child_node = tree.value(child);
				if (child_node.type == editor_asset_node_type_e::folder)
				{
					const size_t saved_len = current_path.size();
					current_path += '/';
					current_path.append(child_node.name.c_str(), child_node.name.size());
					any_visible |= append_folder_rows(child, child_depth, current_path);
					current_path.resize(saved_len);
				}
				child = tree.next_sibling(child);
			}
		}

		const bool should_emit_self = may_emit_self && (!search_active || self_matches_search || any_visible);
		if (may_emit_self && !should_emit_self)
			--_visible_folder_row_count;

		if (should_emit_self)
		{
			bool has_children = any_visible;
			if (will_fold)
			{
				editor_asset_node_handle_t child = tree.first_child(node);
				while (!child.is_null() && !has_children)
				{
					const editor_asset_node_t& child_node = tree.value(child);
					if (child_node.type == editor_asset_node_type_e::folder && (child_node.flags & editor_asset_node_flag_hidden) == 0)
						has_children = true;
					child = tree.next_sibling(child);
				}
			}

			folder_row_t& row = get_or_create_folder_row(my_slot);
			update_folder_row(row, node, asset_node.name.c_str(), depth, path_hash, has_children, will_fold, favourite);
		}

		return should_emit_self || any_visible;
	}

	void editor_panel_assets_t::refresh_asset_grid(bool force)
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t folder		   = get_selected_folder_node();
		const u64						 folder_hash   = find_row_by_hash(_selected_folder_hash) != nullptr ? _selected_folder_hash : 0;

		if (!force && _asset_grid_generation == asset_manager.get_generation() && _asset_grid_folder_hash == folder_hash)
			return;

		_asset_grid_generation	= asset_manager.get_generation();
		_asset_grid_folder_hash = folder_hash;

		clear_asset_grid();

		const ui::layout_out_t& body_out = _ui->get_tree().out(_assets_body_pane_top);
		if (asset_tree.empty() || folder.is_null() || !asset_tree.is_valid(folder) || body_out.size.x <= 0.0f)
			return;

		const editor_theme_t& theme			= editor_theme_t::get();
		const f32			  scale			= ui::get_valid_scale(_ui->get_ui_scale());
		const f32			  body_width	= body_out.size.x / scale;
		const f32			  content_width = math::max(0.0f, body_width - theme.margin_horizontal * 2.0f);
		const vec2f_t		  item_size		= {theme.item_height * 3.5f, theme.item_height * 5.25f};
		const f32			  slot_width	= item_size.x + theme.item_spacing;
		const u32			  column_count	= math::max(1u, static_cast<u32>((content_width + theme.item_spacing) / slot_width));

		u32						   column_index = 0;
		ui::widget_id_t			   row			= NULL_WIDGET;
		editor_asset_node_handle_t child		= asset_tree.first_child(folder);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = asset_tree.value(child);
			if (child_node.type != editor_asset_node_type_e::folder && (child_node.flags & editor_asset_node_flag_hidden) == 0)
			{
				if (!_asset_search_str_lower.empty())
				{
					frame_string_t<char> name_lower;
					name_lower.assign(child_node.name.c_str(), child_node.name.size());
					string_util::to_lower(name_lower);
					if (name_lower.find(_asset_search_str_lower.c_str()) == frame_string_t<char>::npos)
					{
						child = asset_tree.next_sibling(child);
						continue;
					}
				}

				if (_asset_item_style == asset_item_style_e::list)
				{
					append_asset_list_item(child);
					child = asset_tree.next_sibling(child);
					continue;
				}

				if (column_index == 0)
				{
					row = _ui->allocate_widget();
					_ui->set_widget_debug_name(row, "asset_grid_row");
					_ui->get_tree().attach(_assets_body_pane_top, row);
					_ui->get_tree().draw_order(row) = _ui->get_tree().draw_order_const(_assets_body_pane_top) + 1;

					ui::layout_in_t& row_in = _ui->get_tree().in(row);
					row_in.flags			= ui::wf_visible;
					row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
					row_in.size_mode_y		= ui::axis_mode_e::fixed;
					row_in.size_value		= {1.0f, item_size.y};
					row_in.flow				= ui::flow_e::row;
					row_in.child_spacing	= theme.item_spacing;

					_asset_grid_rows.push_back(row);
				}

				append_asset_grid_item(row, child, item_size);
				column_index = (column_index + 1) % column_count;
			}
			child = asset_tree.next_sibling(child);
		}
	}

	void editor_panel_assets_t::clear_asset_grid()
	{
		for (ui::widget_id_t row : _asset_grid_rows)
			_ui->deallocate_widget(row);
		_asset_grid_rows.resize(0);
		_asset_grid_items.resize(0);
		_ui->get_tree().in(_assets_body_pane_top).scroll_offset = {};
	}

	void editor_panel_assets_t::update_current_directory_label()
	{
		const editor_asset_node_handle_t folder = get_selected_folder_node();

		if (folder.is_null())
		{
			_ui->set_widget_text(_assets_body_pane_path, "");
			_ui->get_paint().set_text(
				_assets_body_pane_path,
				_ui->widget_text(_assets_body_pane_path),
				_ui->widget_text_len(_assets_body_pane_path),
				{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text1, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			return;
		}

		const editor_asset_tree_t& tree		   = editor_asset_manager_t::get().get_asset_tree();
		const string_t&			   folder_path = tree.value(folder).full_path;
		SFG_ASSERT(!folder_path.empty());

		const string_t project_directory = file_system_t::get_directory_of_file(editor_project_t::get()._runtime.path.c_str());
		string_t	   relative_path	 = file_system_t::get_relative(project_directory.c_str(), folder_path.c_str());
		if (relative_path.empty())
			relative_path = "assets";

		frame_string_t<char> label;
		label += '/';
		label.append(relative_path.c_str(), relative_path.size());
		_ui->set_widget_text(_assets_body_pane_path, label.c_str());
		_ui->get_paint().set_text(
			_assets_body_pane_path,
			_ui->widget_text(_assets_body_pane_path),
			_ui->widget_text_len(_assets_body_pane_path),
			{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text1, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_assets_t::append_asset_grid_item(ui::widget_id_t row, editor_asset_node_handle_t node, const vec2f_t& item_size)
	{
		ui::ui_context&					 ui			= *_ui;
		ui::layout_tree_t&				 tree		= ui.get_tree();
		ui::paint_layer_t&				 paint		= ui.get_paint();
		const editor_theme_t&			 theme		= editor_theme_t::get();
		const editor_asset_node_t&		 asset_node = editor_asset_manager_t::get().get_asset_tree().value(node);
		const editor_asset_t*			 asset		= asset_node.type == editor_asset_node_type_e::asset ? editor_asset_manager_t::get().find_asset(asset_node.asset_id) : nullptr;
		const editor_asset_descriptor_t* descriptor = nullptr;
		if (asset != nullptr)
		{
			const auto& descriptors	  = editor_asset_manager_t::get().get_asset_descriptors();
			const auto	descriptor_it = descriptors.find(asset->asset_type);
			descriptor				  = descriptor_it != descriptors.end() ? &descriptor_it->second : nullptr;
		}
		const char*	  type_text	 = descriptor != nullptr && !descriptor->display_name.empty() ? descriptor->display_name.c_str() : "File";
		const vec4f_t item_color = descriptor != nullptr ? descriptor->color : theme.color_outline_light;
		const bool	  has_status = asset != nullptr && asset->status != editor_asset_status_e::ok;

		asset_grid_item_t item = {};
		item.node			   = node;

		item.root = ui.allocate_widget();
		ui.set_widget_debug_name(item.root, "asset_grid_item");
		tree.attach(row, item.root);
		tree.draw_order(item.root) = tree.draw_order_const(row);

		ui::layout_in_t& root_in = tree.in(item.root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = item_size;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		ui::vg_rect_paint_t root_rect = {};
		root_rect.fill_color_a		  = theme.color_panel;
		root_rect.fill_color_b		  = theme.color_panel;
		root_rect.rounding			  = theme.item_rounding;
		root_rect.rounding_segs		  = 4;
		paint.set_rect(item.root, root_rect);
		paint.set_hover_color(item.root, theme.color_panel_light);
		paint.set_press_color(item.root, theme.color_light);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_asset_grid_item_clicked;
		ui.get_input().set_listener(item.root, listener);

		item.thumbnail_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.thumbnail_frame, "asset_grid_item_thumbnail");
		tree.attach(item.root, item.thumbnail_frame);
		tree.draw_order(item.thumbnail_frame) = tree.draw_order_const(item.root);

		ui::layout_in_t& thumbnail_in = tree.in(item.thumbnail_frame);
		thumbnail_in.flags			  = ui::wf_visible;
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_value		  = {1.0f, 1.0f};

		ui::vg_rect_paint_t thumbnail_rect = {};
		thumbnail_rect.fill_color_a		   = {1.0f, 1.0f, 1.0f, 1.0f};
		thumbnail_rect.fill_color_b		   = thumbnail_rect.fill_color_a;
		thumbnail_rect.rounding			   = theme.item_rounding;
		thumbnail_rect.rounding_segs	   = 4;
		paint.set_rect(item.thumbnail_frame, thumbnail_rect);

		item.status_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.status_text, "asset_grid_item_status");
		tree.attach(item.thumbnail_frame, item.status_text);
		tree.draw_order(item.status_text) = tree.draw_order_const(item.thumbnail_frame);

		ui::layout_in_t& status_in = tree.in(item.status_text);
		status_in.flags			   = has_status ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : static_cast<u16>(ui::wf_overlay);
		status_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_value		   = {0.0f, 1.0f};
		status_in.anchor_x		   = ui::anchor_e::start;
		status_in.anchor_y		   = ui::anchor_e::end;
		status_in.size_mode_x	   = ui::axis_mode_e::fixed;
		status_in.size_mode_y	   = ui::axis_mode_e::fixed;
		status_in.size_value	   = {theme.item_height, theme.item_height};

		ui.set_widget_text(item.status_text, ICON_WARN);
		paint.set_text(item.status_text,
					   ui.widget_text(item.status_text),
					   ui.widget_text_len(item.status_text),
					   {.font = theme.font_icons, .color = theme.color_accent_warn, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.star_text, "asset_grid_item_star");
		tree.attach(item.thumbnail_frame, item.star_text);
		tree.draw_order(item.star_text) = tree.draw_order_const(item.thumbnail_frame);

		ui::layout_in_t& star_in = tree.in(item.star_text);
		star_in.flags			 = is_asset_favourite(get_asset_guid(node)) ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : static_cast<u16>(ui::wf_overlay);
		star_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_value		 = {1.0f, 1.0f};
		star_in.anchor_x		 = ui::anchor_e::end;
		star_in.anchor_y		 = ui::anchor_e::end;
		star_in.size_mode_x		 = ui::axis_mode_e::fixed;
		star_in.size_mode_y		 = ui::axis_mode_e::fixed;
		star_in.size_value		 = {theme.item_height, theme.item_height};

		ui.set_widget_text(item.star_text, ICON_STAR);
		paint.set_text(item.star_text,
					   ui.widget_text(item.star_text),
					   ui.widget_text_len(item.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.info_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.info_frame, "asset_grid_item_info");
		tree.attach(item.root, item.info_frame);
		tree.draw_order(item.info_frame) = tree.draw_order_const(item.root);

		ui::layout_in_t& info_in = tree.in(item.info_frame);
		info_in.flags			 = ui::wf_visible;
		info_in.child_clip_mode	 = ui::clip_mode_e::cpu_rect;
		info_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		info_in.size_mode_y		 = ui::axis_mode_e::fixed;
		info_in.size_value		 = {1.0f, item_size.y / 3.5f};

		const bool			selected  = _selected_asset_node == item.node;
		ui::vg_rect_paint_t info_rect = {};
		info_rect.fill_color_a		  = selected ? theme.color_accent0 : theme.color_frame;
		info_rect.fill_color_b		  = selected ? theme.color_accent0_dim : theme.color_frame;
		info_rect.gradient			  = selected ? ui::vg_gradient_e::vertical : ui::vg_gradient_e::none;
		paint.set_rect(item.info_frame, info_rect);

		item.color_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.color_frame, "asset_grid_item_color");
		tree.attach(item.root, item.color_frame);
		tree.draw_order(item.color_frame) = tree.draw_order_const(item.root);

		ui::layout_in_t& color_in = tree.in(item.color_frame);
		color_in.flags			  = ui::wf_visible;
		color_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		color_in.size_mode_y	  = ui::axis_mode_e::fill;
		color_in.size_value		  = {1.0f, 1.0f};

		ui::vg_rect_paint_t color_rect = {};
		color_rect.fill_color_a		   = item_color;
		color_rect.fill_color_b		   = item_color;
		paint.set_rect(item.color_frame, color_rect);

		item.label = ui.allocate_widget();
		ui.set_widget_debug_name(item.label, "asset_grid_item_label");
		tree.attach(item.info_frame, item.label);
		tree.draw_order(item.label) = tree.draw_order_const(item.info_frame);

		ui::layout_in_t& label_in = tree.in(item.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.38f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.text_default_px_size};

		ui.set_widget_text(item.label, asset_node.name.c_str());
		paint.set_text(item.label,
					   ui.widget_text(item.label),
					   ui.widget_text_len(item.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.type_label = ui.allocate_widget();
		ui.set_widget_debug_name(item.type_label, "asset_grid_item_type_label");
		tree.attach(item.info_frame, item.type_label);
		tree.draw_order(item.type_label) = tree.draw_order_const(item.info_frame);

		ui::layout_in_t& type_label_in = tree.in(item.type_label);
		type_label_in.flags			   = ui::wf_visible;
		type_label_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		type_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		type_label_in.pos_value		   = {0.5f, 0.70f};
		type_label_in.anchor_x		   = ui::anchor_e::center;
		type_label_in.anchor_y		   = ui::anchor_e::center;
		type_label_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		type_label_in.size_mode_y	   = ui::axis_mode_e::fixed;
		type_label_in.size_value	   = {1.0f, theme.text_small_title_px_size};

		ui.set_widget_text(item.type_label, type_text);
		paint.set_text(item.type_label,
					   ui.widget_text(item.type_label),
					   ui.widget_text_len(item.type_label),
					   {.font = theme.font_title, .color = theme.color_text2, .point_size = theme.text_small_title_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_asset_grid_items.push_back(item);
	}

	void editor_panel_assets_t::append_asset_list_item(editor_asset_node_handle_t node)
	{
		ui::ui_context&					 ui			= *_ui;
		ui::layout_tree_t&				 tree		= ui.get_tree();
		ui::paint_layer_t&				 paint		= ui.get_paint();
		const editor_theme_t&			 theme		= editor_theme_t::get();
		const editor_asset_node_t&		 asset_node = editor_asset_manager_t::get().get_asset_tree().value(node);
		const editor_asset_t*			 asset		= asset_node.type == editor_asset_node_type_e::asset ? editor_asset_manager_t::get().find_asset(asset_node.asset_id) : nullptr;
		const editor_asset_descriptor_t* descriptor = nullptr;
		if (asset != nullptr)
		{
			const auto& descriptors	  = editor_asset_manager_t::get().get_asset_descriptors();
			const auto	descriptor_it = descriptors.find(asset->asset_type);
			descriptor				  = descriptor_it != descriptors.end() ? &descriptor_it->second : nullptr;
		}
		const vec4f_t item_color = descriptor != nullptr ? descriptor->color : theme.color_outline_light;
		const bool	  has_status = asset != nullptr && asset->status != editor_asset_status_e::ok;

		asset_grid_item_t item = {};
		item.node			   = node;

		item.root = ui.allocate_widget();
		ui.set_widget_debug_name(item.root, "asset_list_item");
		tree.attach(_assets_body_pane_top, item.root);
		tree.draw_order(item.root) = tree.draw_order_const(_assets_body_pane_top);

		ui::layout_in_t& root_in = tree.in(item.root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = theme.item_spacing * 0.5f;
		root_in.child_margins	 = {0.0f, theme.item_height * 2.0f, 0.0f, 0.0f};

		const bool			selected  = _selected_asset_node == item.node;
		ui::vg_rect_paint_t root_rect = {};
		root_rect.fill_color_a		  = selected ? theme.color_accent0_dim : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		root_rect.fill_color_b		  = selected ? theme.color_accent0 : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		root_rect.gradient			  = selected ? ui::vg_gradient_e::vertical : ui::vg_gradient_e::none;
		paint.set_rect(item.root, root_rect);
		paint.set_hover_color(item.root, selected ? theme.color_accent0 : theme.color_panel_light);
		paint.set_press_color(item.root, theme.color_light);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_asset_grid_item_clicked;
		ui.get_input().set_listener(item.root, listener);

		item.info_frame = item.root;

		item.color_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.color_frame, "asset_list_item_color");
		tree.attach(item.root, item.color_frame);
		tree.draw_order(item.color_frame) = tree.draw_order_const(item.root);

		ui::layout_in_t& color_in = tree.in(item.color_frame);
		color_in.flags			  = ui::wf_visible;
		color_in.size_mode_x	  = ui::axis_mode_e::fixed;
		color_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		color_in.size_value		  = {theme.border_thickness, 1.0f};

		ui::vg_rect_paint_t color_rect = {};
		color_rect.fill_color_a		   = item_color;
		color_rect.fill_color_b		   = item_color;
		paint.set_rect(item.color_frame, color_rect);

		item.thumbnail_frame = ui.allocate_widget();
		ui.set_widget_debug_name(item.thumbnail_frame, "asset_list_item_thumbnail");
		tree.attach(item.root, item.thumbnail_frame);
		tree.draw_order(item.thumbnail_frame) = tree.draw_order_const(item.root);

		ui::layout_in_t& thumbnail_in = tree.in(item.thumbnail_frame);
		thumbnail_in.flags			  = ui::wf_visible;
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_value		  = {1.0f, 0.85f};
		thumbnail_in.anchor_y		  = ui::anchor_e::center;
		thumbnail_in.pos_value.y	  = 0.5f;
		thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;

		ui::vg_rect_paint_t thumbnail_rect = {};
		thumbnail_rect.fill_color_a		   = {1.0f, 1.0f, 1.0f, 1.0f};
		thumbnail_rect.fill_color_b		   = thumbnail_rect.fill_color_a;
		paint.set_rect(item.thumbnail_frame, thumbnail_rect);

		item.label = ui.allocate_widget();
		ui.set_widget_debug_name(item.label, "asset_list_item_label");
		tree.attach(item.root, item.label);
		tree.draw_order(item.label) = tree.draw_order_const(item.root);

		ui::layout_in_t& label_in = tree.in(item.label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fill;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.text_default_px_size};

		ui.set_widget_text(item.label, asset_node.name.c_str());
		paint.set_text(item.label,
					   ui.widget_text(item.label),
					   ui.widget_text_len(item.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.status_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.status_text, "asset_list_item_status");
		tree.attach(item.root, item.status_text);
		tree.draw_order(item.status_text) = tree.draw_order_const(item.root);

		ui::layout_in_t& status_in = tree.in(item.status_text);
		status_in.flags			   = has_status ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : static_cast<u16>(ui::wf_overlay);
		status_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		status_in.pos_value		   = {1.0f, 0.5f};
		status_in.anchor_x		   = ui::anchor_e::end;
		status_in.anchor_y		   = ui::anchor_e::center;
		status_in.size_mode_x	   = ui::axis_mode_e::fixed;
		status_in.size_mode_y	   = ui::axis_mode_e::fixed;
		status_in.size_value	   = {theme.item_height, theme.item_height};
		const f32 root_w		   = _ui->get_tree().out(_assets_body_pane_top).size.x;
		if (root_w > 0.0f)
			status_in.pos_value.x = 1.0f - theme.item_height / root_w;

		ui.set_widget_text(item.status_text, ICON_WARN);
		paint.set_text(item.status_text,
					   ui.widget_text(item.status_text),
					   ui.widget_text_len(item.status_text),
					   {.font = theme.font_icons, .color = theme.color_accent_warn, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		item.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(item.star_text, "asset_list_item_star");
		tree.attach(item.root, item.star_text);
		tree.draw_order(item.star_text) = tree.draw_order_const(item.root);

		ui::layout_in_t& star_in = tree.in(item.star_text);
		star_in.flags			 = is_asset_favourite(get_asset_guid(node)) ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : static_cast<u16>(ui::wf_overlay);
		star_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_value		 = {1.0f, 0.5f};
		star_in.anchor_x		 = ui::anchor_e::end;
		star_in.anchor_y		 = ui::anchor_e::center;
		star_in.size_mode_x		 = ui::axis_mode_e::fixed;
		star_in.size_mode_y		 = ui::axis_mode_e::fixed;
		star_in.size_value		 = {theme.item_height, theme.item_height};

		ui.set_widget_text(item.star_text, ICON_STAR);
		paint.set_text(item.star_text,
					   ui.widget_text(item.star_text),
					   ui.widget_text_len(item.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_asset_grid_rows.push_back(item.root);
		_asset_grid_items.push_back(item);
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
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		paint.set_rect(row.root, row_rect);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_folder_row_clicked;
		listener.on_double_click	   = on_folder_row_double_clicked;
		ui.get_input().set_listener(row.root, listener);

		row.icon = ui.allocate_widget();
		ui.set_widget_debug_name(row.icon, "asset_folder_row_icon_wrapper");
		tree.attach(row.root, row.icon);
		tree.draw_order(row.icon) = tree.draw_order_const(row.root);

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
		tree.draw_order(row.icon_text) = tree.draw_order_const(row.icon);

		ui::layout_in_t& icon_text_in = tree.in(row.icon_text);
		icon_text_in.flags			  = ui::wf_visible;
		icon_text_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_value		  = {0.5f, 0.5f};
		icon_text_in.anchor_x		  = ui::anchor_e::center;
		icon_text_in.anchor_y		  = ui::anchor_e::center;
		icon_text_in.size_mode_x	  = ui::axis_mode_e::fixed;
		icon_text_in.size_mode_y	  = ui::axis_mode_e::fixed;

		row.star_text = ui.allocate_widget();
		ui.set_widget_debug_name(row.star_text, "asset_folder_row_star");
		tree.attach(row.root, row.star_text);
		tree.draw_order(row.star_text) = tree.draw_order_const(row.root);

		ui::layout_in_t& star_in = tree.in(row.star_text);
		star_in.flags			 = 0;
		star_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		star_in.pos_value		 = {1.0f, 0.5f};
		star_in.anchor_x		 = ui::anchor_e::end;
		star_in.anchor_y		 = ui::anchor_e::center;
		star_in.size_mode_x		 = ui::axis_mode_e::fixed;
		star_in.size_mode_y		 = ui::axis_mode_e::fixed;
		star_in.size_value		 = {theme.item_height, theme.item_height};

		ui.set_widget_text(row.star_text, ICON_STAR);
		paint.set_text(row.star_text,
					   ui.widget_text(row.star_text),
					   ui.widget_text_len(row.star_text),
					   {.font = theme.font_icons, .color = theme.color_accent1, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "asset_folder_row_label");
		tree.attach(row.root, row.label);
		tree.draw_order(row.label) = tree.draw_order_const(row.root);

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

	void editor_panel_assets_t::update_folder_row(folder_row_t& row, editor_asset_node_handle_t node, const char* name, u16 depth, u64 path_hash, bool has_children, bool is_folded, bool is_favourite)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		row.node		 = node;
		row.path_hash	 = path_hash;
		row.depth		 = depth;
		row.has_children = has_children;
		row.is_favourite = is_favourite;

		set_folder_row_visible(row, true);
		update_folder_row_background(row);

		ui::layout_in_t& row_in = tree.in(row.root);
		row_in.child_margins	= {0.0f, theme.item_height, 0.0f, theme.margin_horizontal + static_cast<f32>(depth) * theme.indent_horizontal * ASSETS_FOLDER_INDENT_MULT};
		tree.in(row.icon).flags = has_children ? static_cast<u16>(ui::wf_visible | ui::wf_input) : static_cast<u16>(ui::wf_visible);

		const char* icon = has_children ? (is_folded ? ICON_DD_RIGHT : ICON_DD_DOWN) : "";
		_ui->set_widget_text(row.icon_text, icon);
		paint.set_text(row.icon_text,
					   _ui->widget_text(row.icon_text),
					   _ui->widget_text_len(row.icon_text),
					   {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_ui->set_widget_text(row.label, name);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_assets_t::update_folder_row_background(const folder_row_t& row)
	{
		const editor_theme_t& theme	   = editor_theme_t::get();
		const bool			  selected = _selected_folder_hash != 0 && row.path_hash == _selected_folder_hash;

		ui::vg_rect_paint_t row_rect = {};
		row_rect.fill_color_a		 = selected ? theme.color_accent0 : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
		row_rect.fill_color_b		 = selected ? theme.color_accent0_dim : row_rect.fill_color_a;
		row_rect.gradient			 = ui::vg_gradient_e::vertical;
		row_rect.rounding			 = theme.item_rounding;
		row_rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? theme.color_accent0 : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? theme.color_accent0 : theme.color_light);
	}

	void editor_panel_assets_t::refresh_folder_row_backgrounds()
	{
		for (const folder_row_t& row : _folder_rows)
			update_folder_row_background(row);
	}

	void editor_panel_assets_t::set_folder_row_visible(const folder_row_t& row, bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, row.root, visible, /*input=*/true);
		set_widget_visible(tree, row.icon, visible, /*input=*/false);
		set_widget_visible(tree, row.icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.star_text, visible && row.is_favourite, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
	}

	void editor_panel_assets_t::select_folder_row(u64 path_hash)
	{
		_selected_folder_hash = path_hash;
		clear_asset_grid_selection();
		refresh_folder_row_backgrounds();
		update_current_directory_label();
		refresh_asset_grid(true);
	}

	void editor_panel_assets_t::select_asset_grid_item(editor_asset_node_handle_t node)
	{
		_selected_asset_node = node;
		refresh_asset_grid_item_backgrounds();
	}

	void editor_panel_assets_t::clear_asset_grid_selection()
	{
		if (_selected_asset_node.is_null())
			return;

		_selected_asset_node = {};
		refresh_asset_grid_item_backgrounds();
	}

	void editor_panel_assets_t::refresh_asset_grid_item_backgrounds()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			const bool selected = _selected_asset_node == item.node;

			ui::vg_rect_paint_t info_rect = {};
			if (_asset_item_style == asset_item_style_e::list)
			{
				info_rect.fill_color_a = selected ? theme.color_accent0 : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
				info_rect.fill_color_b = selected ? theme.color_accent0_dim : vec4f_t{0.0f, 0.0f, 0.0f, 0.0f};
				_ui->get_paint().set_hover_color(item.root, selected ? theme.color_accent0 : theme.color_panel_light);
			}
			else
			{
				info_rect.fill_color_a = selected ? theme.color_accent0 : theme.color_frame;
				info_rect.fill_color_b = selected ? theme.color_accent0_dim : theme.color_frame;
			}
			info_rect.gradient = selected ? ui::vg_gradient_e::vertical : ui::vg_gradient_e::none;
			_ui->get_paint().set_rect(item.info_frame, info_rect);
		}
	}

	void editor_panel_assets_t::refresh_asset_favourite_icons()
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			const bool favourite		  = is_asset_favourite(get_asset_guid(item.node));
			tree.in(item.star_text).flags = favourite ? static_cast<u16>(ui::wf_visible | ui::wf_overlay) : static_cast<u16>(ui::wf_overlay);
		}
	}

	void editor_panel_assets_t::toggle_folder_fold(u64 path_hash)
	{
		auto it = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), path_hash);
		if (it != _expanded_folder_hashes.end())
			_expanded_folder_hashes.erase(it);
		else
			_expanded_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_folder_favourite(u64 path_hash)
	{
		auto it = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), path_hash);
		if (it != _favourite_folder_hashes.end())
			_favourite_folder_hashes.erase(it);
		else
			_favourite_folder_hashes.push_back(path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_asset_favourite(sid_t guid)
	{
		auto it = std::find(_favourite_asset_guids.begin(), _favourite_asset_guids.end(), guid);
		if (it != _favourite_asset_guids.end())
			_favourite_asset_guids.erase(it);
		else
			_favourite_asset_guids.push_back(guid);
		refresh_asset_favourite_icons();
	}

	void editor_panel_assets_t::open_create_popup(editor_asset_type_e asset_type, u8 sub_type)
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_create_popup_closed;
		desc.user_data				   = this;
		desc.placeholder			   = "Name";
		desc.pos					   = _action_menu_pos;
		desc.width					   = editor_theme_t::get().item_width;
		_create_popup_asset_type	   = asset_type;
		_create_popup_sub_type		   = sub_type;
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::create_folder(const char* name)
	{
		frame_string_t<char> folder_name = name != nullptr ? name : "";
		if (!editor_directories_t::is_valid_asset_name(folder_name.c_str()))
			return;

		const string_t parent_path = get_action_menu_target_folder_path();
		SFG_ASSERT(!parent_path.empty());

		frame_string_t<char> new_folder_path;
		new_folder_path.assign(parent_path.c_str(), parent_path.size());
		new_folder_path.append(folder_name.c_str(), folder_name.size());
		if (file_system_t::exists(new_folder_path.c_str()))
			return;

		if (!file_system_t::create_directory(new_folder_path.c_str()))
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::request_create_assets(const char* directory, const editor_asset_create_desc_t* descs, u8 desc_count, bool allow_overwrite)
	{
		frame_vector_t<editor_asset_create_desc_t> desc_copy;
		desc_copy.reserve(desc_count);
		for (u8 i = 0; i < desc_count; ++i)
			desc_copy.push_back(descs[i]);

		clear_pending_create_assets();

		_pending_import_directory = directory != nullptr ? directory : "";
		_pending_import_create_descs.reserve(desc_copy.size());
		for (const editor_asset_create_desc_t& desc : desc_copy)
			_pending_import_create_descs.push_back(desc);
		_allow_asset_overwrite = allow_overwrite;

		collect_pending_cook_configs();
		if (_pending_cook_configs.empty())
		{
			submit_create_assets();
			return;
		}

		frame_vector_t<editor_modal_cook_option_desc_t> options;
		options.reserve(_pending_cook_configs.size());
		for (const pending_cook_config_t& pending_config : _pending_cook_configs)
		{
			options.push_back({
				.object	 = pending_config.config.object,
				.title	 = pending_config.config.title,
				.type_id = pending_config.config.type_id,
			});
		}
		_cook_options_modal.set_options(options.data(), static_cast<u16>(options.size()));

		editor_modal_button_desc_t buttons[] = {
			{.text = "Cancel", .callback = on_cook_options_cancelled, .user_data = this},
			{.text = "Import", .callback = on_cook_options_imported, .user_data = this},
		};

		editor_modal_controller_t* modal = editor_modal_controller_t::find(*_ui);
		SFG_ASSERT(modal != nullptr);
		editor_modal_content_desc_t cook_options_content = _cook_options_modal.get_content_desc();
		modal->request_modal("Cook Options", "Configure cook options for the imported assets.", true, buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), &cook_options_content);
	}

	void editor_panel_assets_t::submit_create_assets()
	{
		create_assets(_pending_import_directory.c_str(), _pending_import_create_descs.data(), static_cast<u8>(_pending_import_create_descs.size()));
		clear_pending_create_assets();
	}

	void editor_panel_assets_t::create_assets(const char* directory, const editor_asset_create_desc_t* descs, u8 desc_count)
	{
		if (desc_count == 0)
			return;
		SFG_ASSERT(descs != nullptr);

		string_t parent_path = editor_asset_util_t::normalize_directory(directory);
		if (parent_path.empty())
			return;

		frame_vector_t<editor_asset_t> created_assets;
		created_assets.reserve(desc_count);

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const auto&				descriptors	  = asset_manager.get_asset_descriptors();
		const string_t&			assets_path	  = editor_project_t::get()._runtime.assets_path;
		for (u8 i = 0; i < desc_count; ++i)
		{
			const editor_asset_create_desc_t& desc = descs[i];
			SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);
			SFG_ASSERT(desc.asset_type != editor_asset_type_e::count);

			const string_t& asset_name = desc.name;
			if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
				continue;

			frame_string_t<char> asset_path;
			asset_path.assign(parent_path.c_str(), parent_path.size());
			asset_path.append(asset_name.c_str(), asset_name.size());
			asset_path += ".sfg_asset";
			if (!_allow_asset_overwrite && file_system_t::exists(asset_path.c_str()))
				continue;

			editor_asset_t asset = {};
			asset.version		 = editor_asset_t::VERSION;
			asset.guid			 = desc.guid != NULL_SID ? desc.guid : hashing_t::generate_guid64();
			asset.asset_type	 = desc.asset_type;
			asset.sub_type		 = desc.sub_type;
			if (!desc.source_full_path.empty())
			{
				asset.source_type				= editor_asset_source_type_e::file;
				const string_t source_full_path = file_system_t::get_absolute_path(desc.source_full_path.c_str());
				SFG_ASSERT(file_system_t::exists(source_full_path.c_str()));
				asset.source_relative = editor_asset_util_t::get_source_relative(assets_path.c_str(), source_full_path.c_str());
				if (asset.source_relative.empty())
				{
					const string_t source_extension	  = file_system_t::get_file_extension(source_full_path);
					const string_t target_source_path = editor_asset_util_t::make_unique_source_path(parent_path.c_str(), asset_name.c_str(), source_extension.c_str());
					if (!file_system_t::copy_file(source_full_path.c_str(), target_source_path.c_str()))
						continue;

					SFG_ASSERT(file_system_t::exists(target_source_path.c_str()));
					asset.source_relative = editor_asset_util_t::get_source_relative(assets_path.c_str(), target_source_path.c_str());
					SFG_ASSERT(!asset.source_relative.empty());
				}
			}

			const auto descriptor_it = descriptors.find(desc.asset_type);
			SFG_ASSERT(descriptor_it != descriptors.end());
			const editor_asset_descriptor_t& descriptor = descriptor_it->second;
			SFG_ASSERT(descriptor.create_default != nullptr);

			auto  config_it	  = std::find_if(_pending_cook_configs.begin(), _pending_cook_configs.end(), [&](const pending_cook_config_t& pending_config) { return pending_config.asset_type == desc.asset_type; });
			void* cook_config = config_it != _pending_cook_configs.end() ? config_it->config.object : nullptr;
			if (!descriptor.create_default(asset, parent_path.c_str(), asset_name.c_str(), cook_config))
				continue;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				continue;

			created_assets.push_back(asset);
		}

		if (created_assets.empty())
			return;

		asset_manager.cook_assets(created_assets.data(), static_cast<u32>(created_assets.size()));
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::collect_pending_cook_configs()
	{
		const auto& descriptors = editor_asset_manager_t::get().get_asset_descriptors();
		for (const editor_asset_create_desc_t& desc : _pending_import_create_descs)
		{
			const auto descriptor_it = descriptors.find(desc.asset_type);
			SFG_ASSERT(descriptor_it != descriptors.end());
			const editor_asset_descriptor_t& descriptor = descriptor_it->second;
			if (descriptor.create_cook_config == nullptr)
				continue;

			const auto config_it = std::find_if(_pending_cook_configs.begin(), _pending_cook_configs.end(), [&](const pending_cook_config_t& pending_config) { return pending_config.asset_type == desc.asset_type; });
			if (config_it != _pending_cook_configs.end())
				continue;

			editor_asset_cook_config_desc_t config = descriptor.create_cook_config();
			SFG_ASSERT(config.object != nullptr);
			SFG_ASSERT(config.type_id != 0);
			_pending_cook_configs.push_back({.config = config, .asset_type = desc.asset_type});
		}
	}

	void editor_panel_assets_t::clear_pending_cook_configs()
	{
		for (pending_cook_config_t& pending_config : _pending_cook_configs)
		{
			if (pending_config.config.destroy != nullptr)
				pending_config.config.destroy(pending_config.config.object);
		}
		_pending_cook_configs.resize(0);
	}

	void editor_panel_assets_t::clear_pending_create_assets()
	{
		clear_pending_cook_configs();
		_pending_import_create_descs.resize(0);
		_pending_import_directory.clear();
		_allow_asset_overwrite = false;
	}

	void editor_panel_assets_t::delete_folder()
	{
		const folder_row_t* target_row = find_row_by_hash(_selected_folder_hash);
		SFG_ASSERT(target_row != nullptr);
		SFG_ASSERT(!target_row->node.is_null());

		const editor_asset_tree_t& tree		   = editor_asset_manager_t::get().get_asset_tree();
		const u64				   folder_hash = target_row->path_hash;
		const string_t&			   folder_path = tree.value(target_row->node).full_path;
		SFG_ASSERT(!folder_path.empty());
		if (!file_system_t::delete_directory(folder_path.c_str()))
			return;

		_selected_folder_hash = 0;

		if (auto it = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), folder_hash); it != _favourite_folder_hashes.end())
			_favourite_folder_hashes.erase(it);
		if (auto it = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), folder_hash); it != _expanded_folder_hashes.end())
			_expanded_folder_hashes.erase(it);

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::duplicate_folder()
	{
		const folder_row_t* target_row = find_row_by_hash(_selected_folder_hash);
		SFG_ASSERT(target_row != nullptr);
		SFG_ASSERT(!target_row->node.is_null());

		const editor_asset_tree_t& tree		   = editor_asset_manager_t::get().get_asset_tree();
		const string_t&			   folder_path = tree.value(target_row->node).full_path;
		SFG_ASSERT(!folder_path.empty());
		if (file_system_t::duplicate(folder_path.c_str()).empty())
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::open_rename_popup()
	{
		const folder_row_t* target_row = find_row_by_hash(_selected_folder_hash);
		SFG_ASSERT(target_row != nullptr);
		SFG_ASSERT(!target_row->node.is_null());

		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const editor_asset_tree_t& tree	   = editor_asset_manager_t::get().get_asset_tree();
		const ui::layout_out_t&	   row_out = _ui->get_tree().out(target_row->root);
		const f32				   scale   = ui::get_valid_scale(_ui->get_ui_scale());

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_rename_popup_closed;
		desc.user_data				   = this;
		desc.text					   = tree.value(target_row->node).name.c_str();
		desc.pos					   = row_out.pos;
		desc.width					   = math::max(row_out.size.x / scale, editor_theme_t::get().item_width);
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::rename_folder(const char* name)
	{
		const folder_row_t* target_row = find_row_by_hash(_selected_folder_hash);
		SFG_ASSERT(target_row != nullptr);
		SFG_ASSERT(!target_row->node.is_null());

		string_t new_name = name != nullptr ? name : "";
		if (!editor_directories_t::is_valid_asset_name(new_name.c_str()))
			return;

		const editor_asset_tree_t& tree		= editor_asset_manager_t::get().get_asset_tree();
		const string_t&			   old_name = tree.value(target_row->node).name;
		if (new_name == old_name)
			return;

		const string_t& old_path = tree.value(target_row->node).full_path;
		SFG_ASSERT(!old_path.empty());
		const string_t parent_path = file_system_t::get_directory_of_file(old_path.c_str());
		SFG_ASSERT(!parent_path.empty());

		const string_t new_path = parent_path + new_name;
		if (file_system_t::exists(new_path.c_str()))
			return;

		const u64 old_hash = target_row->path_hash;
		const u64 new_hash = get_folder_hash_after_rename(target_row->node, new_name);
		if (!file_system_t::change_directory_name(old_path.c_str(), new_path.c_str()))
			return;

		_selected_folder_hash = new_hash;
		if (auto it = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), old_hash); it != _favourite_folder_hashes.end())
		{
			_favourite_folder_hashes.erase(it);
			if (std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), new_hash) == _favourite_folder_hashes.end())
				_favourite_folder_hashes.push_back(new_hash);
		}
		if (auto it = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), old_hash); it != _expanded_folder_hashes.end())
		{
			_expanded_folder_hashes.erase(it);
			if (std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), new_hash) == _expanded_folder_hashes.end())
				_expanded_folder_hashes.push_back(new_hash);
		}

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	string_t editor_panel_assets_t::get_action_menu_target_folder_path() const
	{
		const folder_row_t* selected_row = find_row_by_hash(_selected_folder_hash);
		if (selected_row != nullptr && !selected_row->node.is_null())
		{
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			const string_t&			   path = tree.value(selected_row->node).full_path;
			if (!path.empty())
				return path;
		}

		return editor_project_t::get()._runtime.assets_path;
	}

	u64 editor_panel_assets_t::get_folder_hash_after_rename(editor_asset_node_handle_t node, const string_t& name) const
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t root_handle   = asset_manager.get_root_node();
		SFG_ASSERT(!node.is_null());
		SFG_ASSERT(!root_handle.is_null());
		SFG_ASSERT(asset_tree.is_valid(node));

		frame_vector_t<editor_asset_node_handle_t> chain;
		editor_asset_node_handle_t				   current = node;
		while (!current.is_null() && !(current == root_handle))
		{
			chain.push_back(current);
			current = asset_tree.parent(current);
		}

		const editor_asset_node_t& root		 = asset_tree.value(root_handle);
		const string_t&			   root_name = node == root_handle ? name : root.name;

		frame_string_t<char> relative_path;
		relative_path.assign(root_name.c_str(), root_name.size());
		for (size_t i = chain.size(); i-- > 0;)
		{
			const editor_asset_node_t& chain_node = asset_tree.value(chain[i]);
			const string_t&			   segment	  = chain[i] == node ? name : chain_node.name;
			relative_path += '/';
			relative_path.append(segment.c_str(), segment.size());
		}
		return hashing_t::hash_u64(relative_path.c_str(), relative_path.size());
	}

	editor_asset_node_handle_t editor_panel_assets_t::get_selected_folder_node() const
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t root_handle   = asset_manager.get_root_node();
		if (asset_tree.empty() || root_handle.is_null() || !asset_tree.is_valid(root_handle))
			return {};

		const folder_row_t* selected_row = find_row_by_hash(_selected_folder_hash);
		if (selected_row != nullptr && !selected_row->node.is_null() && asset_tree.is_valid(selected_row->node))
			return selected_row->node;

		return {};
	}

	sid_t editor_panel_assets_t::get_asset_guid(editor_asset_node_handle_t node) const
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		if (node.is_null() || !asset_tree.is_valid(node))
			return NULL_SID;

		const editor_asset_node_t& asset_node = asset_tree.value(node);
		if (asset_node.type != editor_asset_node_type_e::asset)
			return NULL_SID;

		const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
		return asset != nullptr ? asset->guid : NULL_SID;
	}

	bool editor_panel_assets_t::is_asset_favourite(sid_t guid) const
	{
		return guid != NULL_SID && std::find(_favourite_asset_guids.begin(), _favourite_asset_guids.end(), guid) != _favourite_asset_guids.end();
	}

	bool editor_panel_assets_t::has_favourite_asset_descendant(editor_asset_node_handle_t node) const
	{
		const editor_asset_tree_t& tree	 = editor_asset_manager_t::get().get_asset_tree();
		editor_asset_node_handle_t child = tree.first_child(node);
		while (!child.is_null())
		{
			const editor_asset_node_t& child_node = tree.value(child);
			if ((child_node.flags & editor_asset_node_flag_hidden) == 0)
			{
				if (child_node.type == editor_asset_node_type_e::asset && is_asset_favourite(get_asset_guid(child)))
					return true;
				if (child_node.type == editor_asset_node_type_e::folder && has_favourite_asset_descendant(child))
					return true;
			}
			child = tree.next_sibling(child);
		}
		return false;
	}

	const editor_panel_assets_t::folder_row_t* editor_panel_assets_t::find_row_by_hash(u64 path_hash) const
	{
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if (row.path_hash == path_hash)
				return &row;
		}
		return nullptr;
	}

	const editor_panel_assets_t::folder_row_t* editor_panel_assets_t::find_row_by_widget(ui::widget_id_t id, bool match_icon) const
	{
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			const folder_row_t& row = _folder_rows[i];
			if ((match_icon ? row.icon : row.root) == id)
				return &row;
		}
		return nullptr;
	}

	const editor_panel_assets_t::asset_grid_item_t* editor_panel_assets_t::find_asset_grid_item_by_widget(ui::widget_id_t id) const
	{
		for (const asset_grid_item_t& item : _asset_grid_items)
		{
			if (item.root == id)
				return &item;
		}
		return nullptr;
	}

	void editor_panel_assets_t::on_filter_popup_pressed(u16 value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._favourites_only = value == ASSETS_FILTER_ID_FAVOURITES;
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_filter_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel.open_filter_popup();
	}

	void editor_panel_assets_t::on_import_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		vector_t<string_t> paths;
		process::select_files("Import Assets", "glb;png;jpg;jpeg;mp3;ttf", paths);
		if (!paths.empty())
			panel.import_assets(paths);
	}

	void editor_panel_assets_t::on_refresh_button_pressed(bool, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		editor_asset_manager_t::get().rescan(editor_project_t::get()._runtime.assets_path);
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_assets_t&		 panel			= *static_cast<editor_panel_assets_t*>(user_data);
		const create_asset_command_t create_command = asset_type_from_create_command(command);
		if (create_command.asset_type != editor_asset_type_e::invalid)
		{
			panel._create_popup_asset_type = create_command.asset_type;
			panel._create_popup_sub_type   = create_command.sub_type;
			panel._create_popup_pending	   = true;
			return;
		}

		switch (command)
		{
		case assets_action_menu_create_folder:
			panel._create_popup_asset_type = editor_asset_type_e::invalid;
			panel._create_popup_sub_type   = 0;
			panel._create_popup_pending	   = true;
			return;
		case assets_action_menu_delete:
			panel.delete_folder();
			return;
		case assets_action_menu_duplicate:
			panel.duplicate_folder();
			return;
		case assets_action_menu_rename:
			panel._rename_popup_pending = true;
			return;
		case assets_action_menu_toggle_favourite:
			if (panel.find_row_by_hash(panel._selected_folder_hash) != nullptr)
				panel.toggle_folder_favourite(panel._selected_folder_hash);
			return;
		case assets_action_menu_open_directory: {
			const string_t folder_path = panel.get_action_menu_target_folder_path();
			if (!folder_path.empty())
				process::open_directory(folder_path.c_str());
			return;
		}
		default:
			return;
		}
	}

	void editor_panel_assets_t::on_asset_action_menu_command(u16 command, void* user_data)
	{
		if (command != assets_item_action_menu_toggle_favourite)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		const sid_t			   guid	 = panel.get_asset_guid(panel._selected_asset_node);
		if (guid != NULL_SID)
			panel.toggle_asset_favourite(guid);
	}

	void editor_panel_assets_t::on_action_menu_closed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (panel._create_popup_pending)
		{
			const editor_asset_type_e asset_type = panel._create_popup_asset_type;
			const u8				  sub_type	 = panel._create_popup_sub_type;
			panel._create_popup_pending			 = false;
			panel.open_create_popup(asset_type, sub_type);
			return;
		}

		if (!panel._rename_popup_pending)
			return;

		panel._rename_popup_pending = false;
		panel.open_rename_popup();
	}

	void editor_panel_assets_t::on_create_popup_closed(const char* value, void* user_data)
	{
		editor_panel_assets_t&	  panel		 = *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_type_e asset_type = panel._create_popup_asset_type;
		const u8				  sub_type	 = panel._create_popup_sub_type;
		panel._create_popup_asset_type		 = editor_asset_type_e::invalid;
		panel._create_popup_sub_type		 = 0;
		if (asset_type == editor_asset_type_e::invalid)
		{
			panel.create_folder(value);
			return;
		}

		const string_t					 directory = panel.get_action_menu_target_folder_path();
		const editor_asset_create_desc_t desc	   = {
				 .name		 = value != nullptr ? value : "",
				 .asset_type = asset_type,
				 .sub_type	 = sub_type,
		 };
		panel.request_create_assets(directory.c_str(), &desc, 1, false);
	}

	void editor_panel_assets_t::on_rename_popup_closed(const char* value, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->rename_folder(value);
	}

	void editor_panel_assets_t::on_import_overwrite_confirmed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.request_create_assets(panel._pending_import_directory.c_str(), panel._pending_import_create_descs.data(), static_cast<u8>(panel._pending_import_create_descs.size()), true);
	}

	void editor_panel_assets_t::on_import_overwrite_cancelled(void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->clear_pending_create_assets();
	}

	void editor_panel_assets_t::on_cook_options_imported(void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->submit_create_assets();
	}

	void editor_panel_assets_t::on_cook_options_cancelled(void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->clear_pending_create_assets();
	}

	void editor_panel_assets_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._search_str		= value != nullptr ? value : "";
		panel._search_str_lower = panel._search_str;
		string_util::to_lower(panel._search_str_lower);
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_asset_search_changed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._asset_search_str		  = value != nullptr ? value : "";
		panel._asset_search_str_lower = panel._asset_search_str;
		string_util::to_lower(panel._asset_search_str_lower);
		panel.refresh_asset_grid(true);
	}

	u16 editor_panel_assets_t::get_selected_item_style(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		return panel._asset_item_style == asset_item_style_e::list ? ASSETS_ITEM_STYLE_ID_LIST : ASSETS_ITEM_STYLE_ID_GRID;
	}

	void editor_panel_assets_t::on_item_style_pressed(u16 value, void* user_data)
	{
		editor_panel_assets_t&	 panel = *static_cast<editor_panel_assets_t*>(user_data);
		const asset_item_style_e style = value == ASSETS_ITEM_STYLE_ID_LIST ? asset_item_style_e::list : asset_item_style_e::grid;
		if (panel._asset_item_style == style)
			return;

		panel._asset_item_style = style;
		panel.refresh_asset_grid(true);
	}

	void editor_panel_assets_t::on_assets_body_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (id == panel._assets_left_pane_body)
			panel.select_folder_row(0);
		else
			panel.clear_asset_grid_selection();

		if (btn != ui::mouse_button_e::right)
			return;

		if (id == panel._assets_left_pane_body)
			panel.open_action_menu(pos);
	}

	void editor_panel_assets_t::on_assets_body_wheel(ui::input_router_t&, ui::widget_id_t id, f32 delta, void* user_data)
	{
		editor_panel_assets_t&	panel  = *static_cast<editor_panel_assets_t*>(user_data);
		ui::layout_tree_t&		tree   = panel._ui->get_tree();
		const ui::widget_id_t	target = id == panel._assets_body_pane_top ? panel._assets_body_pane_top : panel._assets_left_pane_body;
		ui::layout_in_t&		in	   = tree.in(target);
		const ui::layout_out_t& out	   = tree.out(target);
		in.scroll_offset.y			   = math::clamp(in.scroll_offset.y + delta * ASSETS_SCROLL_WHEEL_STEP, -out.max_scroll.y, 0.0f);
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
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (panel._asset_tree_generation != asset_manager.get_generation())
			panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_asset_grid_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_assets_t&		  panel			= *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (panel._asset_grid_generation != asset_manager.get_generation())
			panel.refresh_asset_grid(false);
	}

	void editor_panel_assets_t::on_asset_grid_item_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t&		   panel = *static_cast<editor_panel_assets_t*>(user_data);
		const asset_grid_item_t* const item	 = panel.find_asset_grid_item_by_widget(id);
		if (item == nullptr)
			return;

		panel.select_asset_grid_item(item->node);
		if (btn == ui::mouse_button_e::right)
			panel.open_asset_action_menu(pos);
	}

	void editor_panel_assets_t::on_folder_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		const folder_row_t* const row = panel.find_row_by_widget(id, /*match_icon=*/true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			panel.select_folder_row(row->path_hash);
			panel.open_action_menu(pos);
		}
		else if (row->has_children)
			panel.toggle_folder_fold(row->path_hash);
	}

	void editor_panel_assets_t::on_folder_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		const folder_row_t* const row = panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			panel.select_folder_row(row->path_hash);
			panel.open_action_menu(pos);
		}
		else
			panel.select_folder_row(row->path_hash);
	}

	void editor_panel_assets_t::on_folder_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_assets_t&	  panel = *static_cast<editor_panel_assets_t*>(user_data);
		const folder_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr || !row->has_children)
			return;
		panel.toggle_folder_fold(row->path_hash);
	}
}
