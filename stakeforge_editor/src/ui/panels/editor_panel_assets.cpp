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
#include "editor_directories.hpp"
#include "editor_settings.hpp"
#include "ui/editor_action_menu_controller.hpp"
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
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ASSETS_PANE_SPLIT_MIN			   0.15f
#define ASSETS_PANE_SPLIT_MAX			   0.35f
#define ASSETS_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define ASSETS_FOLDER_INDENT_MULT		   2.0f
#define ASSETS_SCROLL_WHEEL_STEP		   32.0f
#define ASSETS_INITIAL_ROW_CAPACITY		   64
#define ASSETS_FILTER_ID_ALL			   0
#define ASSETS_FILTER_ID_FAVOURITES		   1

	namespace
	{
		enum assets_action_menu_command_e : u16
		{
			assets_action_menu_create_folder	= 1,
			assets_action_menu_delete			= 2,
			assets_action_menu_duplicate		= 3,
			assets_action_menu_toggle_favourite = 4,
			assets_action_menu_open_directory	= 5,
			assets_action_menu_rename			= 6,
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_ANIMATION_ROWS[] = {
			{.text = "Animation State Machine"},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GRAPHICS_ROWS[] = {
			{.text = "Shader"},
			{.text = "Texture Sampler"},
			{.text = "Material"},
			{.text = "Particle Properties"},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GAMEPLAY_ROWS[] = {
			{.text = "C# Script"},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_PHYSICS_ROWS[] = {
			{.text = "Physical Material"},
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

		bool contains_hash(const vector_t<u64>& set, u64 hash)
		{
			for (u64 h : set)
			{
				if (h == hash)
					return true;
			}
			return false;
		}

		void toggle_hash(vector_t<u64>& set, u64 hash)
		{
			for (auto it = set.begin(); it != set.end(); ++it)
			{
				if (*it != hash)
					continue;
				set.erase(it);
				return;
			}
			set.push_back(hash);
		}

		void remove_hash(vector_t<u64>& set, u64 hash)
		{
			for (auto it = set.begin(); it != set.end(); ++it)
			{
				if (*it != hash)
					continue;
				set.erase(it);
				return;
			}
		}

		u64 combine_path_hash(u64 parent_hash, const string_t& child_name)
		{
			const u64 with_sep = hashing_t::hash_fnv_1a64(parent_hash, "/", 1);
			return hashing_t::hash_fnv_1a64(with_sep, child_name.c_str(), child_name.size());
		}

		bool name_contains_lower(const string_t& name_lower, const string_t& needle_lower)
		{
			return name_lower.find(needle_lower) != string_t::npos;
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
		refresh_button_config.icon						  = ICON_ROTATE;
		refresh_button_config.toggled_icon				  = ICON_ROTATE;
		refresh_button_config.tooltip					  = "Refresh";
		refresh_button_config.on_clicked				  = on_refresh_button_pressed;
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
		body_pane_in.size_mode_x	  = ui::axis_mode_e::fill;
		body_pane_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		body_pane_in.size_value		  = {1.0f, 1.0f};

		_assets_body_pane_top = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_body_pane_top, "assets_body_pane_top");
		tree.attach(_assets_body_pane, _assets_body_pane_top);

		ui::layout_in_t& body_top_in = tree.in(_assets_body_pane_top);
		body_top_in.flags			 = ui::wf_visible;
		body_top_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_top_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_top_in.size_value		 = {1.0f, 1.0f};

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

		_folder_rows.reserve(ASSETS_INITIAL_ROW_CAPACITY);
		_expanded_folder_hashes.reserve(256);
		_favourite_folder_hashes.reserve(256);
		apply_pane_split();
		refresh_folder_rows();
	}

	void editor_panel_assets_t::uninit()
	{
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

		_folder_rows.clear();
		_expanded_folder_hashes.clear();
		_favourite_folder_hashes.clear();

		editor_panel_t::uninit();
	}

	void editor_panel_assets_t::serialize(nlohmann::json& j) const
	{
		j							= nlohmann::json::object();
		j["pane_split"]				= _pane_split;
		j["favourites_only"]		= _favourites_only;
		j["search_str"]				= _search_str;
		j["favourites"]				= _favourite_folder_hashes;
		j["thumbnail_slider_value"] = _thumbnail_slider_value;
	}

	void editor_panel_assets_t::deserialize(const nlohmann::json& j)
	{
		_pane_split				 = math::clamp(j.value<f32>("pane_split", _pane_split), ASSETS_PANE_SPLIT_MIN, ASSETS_PANE_SPLIT_MAX);
		_favourites_only		 = j.value<bool>("favourites_only", false);
		_search_str				 = j.value<string_t>("search_str", {});
		_favourite_folder_hashes = j.value<vector_t<u64>>("favourites", {});
		_thumbnail_slider_value	 = math::clamp(j.value<f32>("thumbnail_slider_value", _thumbnail_slider_value), 0.2f, 1.0f);
		_search_str_lower		 = _search_str;
		string_util::to_lower(_search_str_lower);
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

	void editor_panel_assets_t::open_action_menu(const vec2f_t& pos, editor_asset_node_handle_t folder, u64 folder_hash)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_folder		  = folder;
		_action_menu_folder_hash  = folder.is_null() ? 0 : folder_hash;
		const bool folder_context = !folder.is_null();

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

	void editor_panel_assets_t::refresh_folder_rows()
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  asset_tree	= asset_manager.get_asset_tree();
		_asset_tree_generation						= asset_manager.get_generation();
		_visible_folder_row_count					= 0;

		const editor_asset_node_handle_t root_handle = asset_manager.get_root_node();
		if (!asset_tree.empty() && !root_handle.is_null() && asset_tree.is_valid(root_handle))
		{
			const editor_asset_node_t& root		 = asset_tree.value(root_handle);
			const u64				   root_hash = hashing_t::hash_fnv_1a64(hashing_t::FNV_1A_64_OFFSET, root.name.c_str(), root.name.size());
			append_folder_rows(root_handle, 0, root_hash);
		}

		for (size_t i = _visible_folder_row_count; i < _folder_rows.size(); ++i)
			set_folder_row_visible(_folder_rows[i], false);
	}

	bool editor_panel_assets_t::append_folder_rows(editor_asset_node_handle_t node, u16 depth, u64 path_hash)
	{
		const editor_asset_tree_t& tree		  = editor_asset_manager_t::get().get_asset_tree();
		const editor_asset_node_t& asset_node = tree.value(node);
		if ((asset_node.flags & editor_asset_node_flag_hidden) != 0)
			return false;

		const bool search_active = !_search_str_lower.empty();
		const bool promoted		 = (asset_node.flags & editor_asset_node_flag_promoted) != 0;
		const bool favourite	 = contains_hash(_favourite_folder_hashes, path_hash);
		const bool passes_filter = !_favourites_only || favourite;

		bool self_matches_search = !search_active;
		if (search_active)
		{
			string_t name_lower = asset_node.name;
			string_util::to_lower(name_lower);
			self_matches_search = name_contains_lower(name_lower, _search_str_lower);
		}

		const bool is_expanded	 = contains_hash(_expanded_folder_hashes, path_hash);
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
					const u64 child_hash = combine_path_hash(path_hash, child_node.name);
					any_visible |= append_folder_rows(child, child_depth, child_hash);
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
		tree.draw_order(row.star_text) = tree.draw_order_const(row.root) + 2;

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
		set_widget_visible(tree, row.root, visible, /*input=*/true);
		set_widget_visible(tree, row.icon, visible, /*input=*/false);
		set_widget_visible(tree, row.icon_text, visible, /*input=*/false);
		set_widget_visible(tree, row.star_text, visible && row.is_favourite, /*input=*/false);
		set_widget_visible(tree, row.label, visible, /*input=*/false);
	}

	void editor_panel_assets_t::select_folder_row(u64 path_hash)
	{
		_selected_folder_hash = path_hash;
		refresh_folder_row_backgrounds();
	}

	void editor_panel_assets_t::toggle_folder_fold(u64 path_hash)
	{
		toggle_hash(_expanded_folder_hashes, path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::toggle_folder_favourite(u64 path_hash)
	{
		toggle_hash(_favourite_folder_hashes, path_hash);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::create_folder()
	{
		string_t parent_path = get_action_menu_target_folder_path();
		if (parent_path.empty())
			return;

		if (parent_path.back() != '/')
			parent_path += '/';

		string_t new_folder_path = parent_path + "New Folder";
		u32		 copy_index		 = 1;
		while (file_system_t::exists(new_folder_path.c_str()))
			new_folder_path = parent_path + "New Folder " + std::to_string(copy_index++);

		if (!file_system_t::create_directory(new_folder_path.c_str()))
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (asset_manager.rescan(editor_settings_t::get().get_project()))
			refresh_folder_rows();
	}

	void editor_panel_assets_t::delete_folder()
	{
		SFG_ASSERT(!_action_menu_folder.is_null());

		const string_t folder_path = get_folder_absolute_path(_action_menu_folder);
		SFG_ASSERT(!folder_path.empty());
		if (!file_system_t::delete_directory(folder_path.c_str()))
			return;

		if (_selected_folder_hash == _action_menu_folder_hash)
			_selected_folder_hash = 0;

		remove_hash(_favourite_folder_hashes, _action_menu_folder_hash);
		remove_hash(_expanded_folder_hashes, _action_menu_folder_hash);

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (asset_manager.rescan(editor_settings_t::get().get_project()))
			refresh_folder_rows();
	}

	void editor_panel_assets_t::duplicate_folder()
	{
		SFG_ASSERT(!_action_menu_folder.is_null());

		const string_t folder_path = get_folder_absolute_path(_action_menu_folder);
		SFG_ASSERT(!folder_path.empty());
		if (file_system_t::duplicate(folder_path.c_str()).empty())
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (asset_manager.rescan(editor_settings_t::get().get_project()))
			refresh_folder_rows();
	}

	void editor_panel_assets_t::open_rename_popup()
	{
		SFG_ASSERT(!_action_menu_folder.is_null());

		const folder_row_t* target_row = nullptr;
		for (u32 i = 0; i < _visible_folder_row_count && i < _folder_rows.size(); ++i)
		{
			if (_folder_rows[i].path_hash == _action_menu_folder_hash)
			{
				target_row = &_folder_rows[i];
				break;
			}
		}
		SFG_ASSERT(target_row != nullptr);

		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const editor_asset_tree_t& tree	   = editor_asset_manager_t::get().get_asset_tree();
		const ui::layout_out_t&	   row_out = _ui->get_tree().out(target_row->root);
		const f32				   scale   = ui::get_valid_scale(_ui->get_ui_scale());

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_rename_popup_closed;
		desc.user_data				   = this;
		desc.text					   = tree.value(_action_menu_folder).name.c_str();
		desc.pos					   = row_out.pos;
		desc.width					   = math::max(row_out.size.x / scale, editor_theme_t::get().item_width);
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::rename_folder(const char* name)
	{
		SFG_ASSERT(!_action_menu_folder.is_null());

		string_t new_name = name != nullptr ? name : "";
		if (new_name.empty())
			return;

		for (char c : new_name)
		{
			if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '\"' || c == '<' || c == '>' || c == '|')
				return;
		}

		const editor_asset_tree_t& tree		= editor_asset_manager_t::get().get_asset_tree();
		const string_t			   old_name = tree.value(_action_menu_folder).name;
		if (new_name == old_name)
			return;

		const string_t old_path = get_folder_absolute_path(_action_menu_folder);
		SFG_ASSERT(!old_path.empty());
		const string_t parent_path = file_system_t::get_directory_of_file(old_path.c_str());
		SFG_ASSERT(!parent_path.empty());

		const string_t new_path = parent_path + new_name;
		if (file_system_t::exists(new_path.c_str()))
			return;

		const u64 old_hash = _action_menu_folder_hash;
		const u64 new_hash = get_folder_hash_after_rename(_action_menu_folder, new_name);
		if (!file_system_t::change_directory_name(old_path.c_str(), new_path.c_str()))
			return;

		if (_selected_folder_hash == old_hash)
			_selected_folder_hash = new_hash;
		if (contains_hash(_favourite_folder_hashes, old_hash))
		{
			remove_hash(_favourite_folder_hashes, old_hash);
			if (!contains_hash(_favourite_folder_hashes, new_hash))
				_favourite_folder_hashes.push_back(new_hash);
		}
		if (contains_hash(_expanded_folder_hashes, old_hash))
		{
			remove_hash(_expanded_folder_hashes, old_hash);
			if (!contains_hash(_expanded_folder_hashes, new_hash))
				_expanded_folder_hashes.push_back(new_hash);
		}
		_action_menu_folder_hash = new_hash;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (asset_manager.rescan(editor_settings_t::get().get_project()))
			refresh_folder_rows();
	}

	string_t editor_panel_assets_t::get_action_menu_target_folder_path() const
	{
		if (!_action_menu_folder.is_null())
			return get_folder_absolute_path(_action_menu_folder);

		if (_selected_folder_hash != 0)
		{
			for (const folder_row_t& row : _folder_rows)
			{
				if (row.path_hash != _selected_folder_hash || row.node.is_null())
					continue;
				const string_t selected_path = get_folder_absolute_path(row.node);
				if (!selected_path.empty())
					return selected_path;
				break;
			}
		}

		return editor_directories_t::get_project_assets_directory(editor_settings_t::get().get_project());
	}

	string_t editor_panel_assets_t::get_folder_absolute_path(editor_asset_node_handle_t node) const
	{
		const editor_asset_manager_t&	 asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&		 asset_tree	   = asset_manager.get_asset_tree();
		const editor_asset_node_handle_t root_handle   = asset_manager.get_root_node();
		if (node.is_null() || !asset_tree.is_valid(node) || root_handle.is_null())
			return {};

		string_t assets_path = editor_directories_t::get_project_assets_directory(editor_settings_t::get().get_project());
		if (assets_path.empty())
			return {};
		if (assets_path.back() != '/')
			assets_path += '/';

		if (node == root_handle)
			return assets_path;

		frame_vector_t<editor_asset_node_handle_t> chain;
		editor_asset_node_handle_t				   current = node;
		while (!current.is_null() && !(current == root_handle))
		{
			chain.push_back(current);
			current = asset_tree.parent(current);
		}

		string_t result = assets_path;
		for (size_t i = chain.size(); i-- > 0;)
		{
			result += asset_tree.value(chain[i]).name;
			if (i != 0)
				result += '/';
		}
		return result;
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
		u64						   hash		 = hashing_t::hash_fnv_1a64(hashing_t::FNV_1A_64_OFFSET, root_name.c_str(), root_name.size());
		for (size_t i = chain.size(); i-- > 0;)
		{
			const editor_asset_node_t& chain_node = asset_tree.value(chain[i]);
			hash								  = combine_path_hash(hash, chain[i] == node ? name : chain_node.name);
		}
		return hash;
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

	void editor_panel_assets_t::on_filter_popup_pressed(u16 value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._favourites_only		 = value == ASSETS_FILTER_ID_FAVOURITES;
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
		if (editor_asset_manager_t::get().rescan(editor_settings_t::get().get_project()))
			panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		switch (command)
		{
		case assets_action_menu_create_folder:
			panel.create_folder();
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
			if (!panel._action_menu_folder.is_null())
				panel.toggle_folder_favourite(panel._action_menu_folder_hash);
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

	void editor_panel_assets_t::on_action_menu_closed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (!panel._rename_popup_pending)
			return;

		panel._rename_popup_pending = false;
		panel.open_rename_popup();
	}

	void editor_panel_assets_t::on_rename_popup_closed(const char* value, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->rename_folder(value);
	}

	void editor_panel_assets_t::on_search_changed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel._search_str			 = value != nullptr ? value : "";
		panel._search_str_lower		 = panel._search_str;
		string_util::to_lower(panel._search_str_lower);
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
		panel.open_action_menu(pos, {}, 0);
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
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		if (panel._asset_tree_generation != asset_manager.get_generation())
			panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_folder_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t&	  panel = *static_cast<editor_panel_assets_t*>(user_data);
		const folder_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
			panel.open_action_menu(pos, row->node, row->path_hash);
		else if (row->has_children)
			panel.toggle_folder_fold(row->path_hash);
	}

	void editor_panel_assets_t::on_folder_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t&	  panel = *static_cast<editor_panel_assets_t*>(user_data);
		const folder_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
			panel.open_action_menu(pos, row->node, row->path_hash);
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
