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
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "assets/editor_asset_manager.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_ANIMATION_ROWS[] = {
			{.text = "Animation Graph", .command = assets_action_menu_create_animation_graph},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GRAPHICS_ROWS[] = {
			{.text = "Lit Shader", .command = assets_action_menu_create_lit_shader},
			{.text = "Unlit Shader", .command = assets_action_menu_create_unlit_shader},
			{.text = "Post Process Shader", .command = assets_action_menu_create_post_process_shader},
			{.text = "UI Shader", .command = assets_action_menu_create_ui_shader},
			{.text = "UI Text Shader", .command = assets_action_menu_create_ui_text_shader},
			{.text = "Skybox Shader", .command = assets_action_menu_create_skybox_shader},
			{.text = "Sprite Lit Shader", .command = assets_action_menu_create_sprite_lit_shader},
			{.text = "Sprite Unlit Shader", .command = assets_action_menu_create_sprite_unlit_shader},
			{.text = "Particle Shader", .command = assets_action_menu_create_particle_shader},
			{.text = "Texture Sampler", .command = assets_action_menu_create_texture_sampler},
			{.text = "Curve", .command = assets_action_menu_create_curve},
			{.text = "Opaque Material", .command = assets_action_menu_create_opaque_material},
			{.text = "Opaque Unlit Material", .command = assets_action_menu_create_opaque_unlit_material},
			{.text = "Transparent Material", .command = assets_action_menu_create_transparent_material},
			{.text = "Transparent Unlit Material", .command = assets_action_menu_create_transparent_unlit_material},
			{.text = "Skybox Material", .command = assets_action_menu_create_skybox_material},
			{.text = "Sprite Lit Material", .command = assets_action_menu_create_sprite_lit_material},
			{.text = "Sprite Unlit Material", .command = assets_action_menu_create_sprite_unlit_material},
			{.text = "Particle Material", .command = assets_action_menu_create_particle_material},
			{.text = "Post Process Material", .command = assets_action_menu_create_post_process_material},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_GAMEPLAY_ROWS[] = {
			{.text = "C# Component", .command = assets_action_menu_create_csharp_component},
			{.text = "C# World Script", .command = assets_action_menu_create_csharp_world_script},
			{.text = "C# Class", .command = assets_action_menu_create_csharp_class},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_PHYSICS_ROWS[] = {
			{.text = "Physical Material", .command = assets_action_menu_create_physical_material},
			{.text = "Ragdoll", .command = assets_action_menu_create_ragdoll},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_CREATE_ROWS[] = {
			{.text = "Folder", .command = assets_action_menu_create_folder},
			{.text = "World", .command = assets_action_menu_create_world},
			{.text = "Animation", .children = ASSETS_ACTION_MENU_ANIMATION_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_ANIMATION_ROWS) / sizeof(ASSETS_ACTION_MENU_ANIMATION_ROWS[0]))},
			{.text = "Graphics", .children = ASSETS_ACTION_MENU_GRAPHICS_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_GRAPHICS_ROWS) / sizeof(ASSETS_ACTION_MENU_GRAPHICS_ROWS[0]))},
			{.text = "Gameplay", .children = ASSETS_ACTION_MENU_GAMEPLAY_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_GAMEPLAY_ROWS) / sizeof(ASSETS_ACTION_MENU_GAMEPLAY_ROWS[0]))},
			{.text = "Physics", .children = ASSETS_ACTION_MENU_PHYSICS_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_PHYSICS_ROWS) / sizeof(ASSETS_ACTION_MENU_PHYSICS_ROWS[0]))},
		};

		editor_action_menu_row_desc_t ASSETS_ACTION_MENU_ROWS[] = {
			{.text = "Create", .children = ASSETS_ACTION_MENU_CREATE_ROWS, .child_count = static_cast<u16>(sizeof(ASSETS_ACTION_MENU_CREATE_ROWS) / sizeof(ASSETS_ACTION_MENU_CREATE_ROWS[0]))},
			{.text = "Import", .command = assets_action_menu_import},
			{.text = "Import ORM Texture", .command = assets_action_menu_import_orm_texture},
			{.text = "Import Sprite", .command = assets_action_menu_import_sprite},
			{.text = "Delete", .shortcut = "DEL", .command = assets_action_menu_delete},
			{.text = "Duplicate", .shortcut = "CTRL+D", .command = assets_action_menu_duplicate},
			{.text = "Rename", .shortcut = "F2", .command = assets_action_menu_rename},
			{.text = "Toggle Favourite", .icon = ICON_STAR, .command = assets_action_menu_toggle_favourite, .has_icon_color = true},
			{.text = "Open In OS", .command = assets_action_menu_open_directory},
			{.text = "Open C# Project", .command = assets_action_menu_open_csharp_project},
			{.text = "Recompile C# Project", .command = assets_action_menu_recompile_csharp_project},
		};

		editor_action_menu_row_desc_t ASSETS_ITEM_ACTION_MENU_ROWS[] = {
			{.text = "Rename", .shortcut = "F2", .command = assets_item_action_menu_rename},
			{.text = "Fix Integrity", .command = assets_item_action_menu_fix_integrity},
			{.text = "Duplicate", .shortcut = "CTRL+D", .command = assets_item_action_menu_duplicate},
			{.text = "Delete", .shortcut = "DEL", .command = assets_item_action_menu_delete},
			{.text = "Show in OS", .command = assets_item_action_menu_open_directory},
			{.text = "Toggle Favourite", .icon = ICON_STAR, .command = assets_item_action_menu_toggle_favourite, .has_icon_color = true},
			{.text = "Open C# Project", .command = assets_action_menu_open_csharp_project},
			{.text = "Recompile C# Project", .command = assets_action_menu_recompile_csharp_project},
		};
	}

	void editor_panel_assets_t::open_filter_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);

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

	void editor_panel_assets_t::open_action_menu(const vec2f_t& pos, bool allow_folder_actions)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);

		_create_asset_popup_command						= 0;
		_create_folder_popup_pending					= false;
		_action_menu_pos								= pos;
		const editor_asset_manager_t&	 asset_manager	= editor_asset_manager_t::get();
		const editor_asset_tree_t&		 tree			= asset_manager.get_asset_tree();
		const editor_asset_node_handle_t root_handle	= asset_manager.get_root_node();
		const bool						 folder_context = !_selected_folder_hashes.empty() && !_selected_folder_node.is_null() && tree.is_valid(_selected_folder_node) && !(_selected_folder_node == root_handle);
		const bool						 folder_actions = allow_folder_actions && folder_context;
		const bool						 multi_selected = _selected_folder_hashes.size() > 1;

		ASSETS_ACTION_MENU_ROWS[0].disabled	  = multi_selected;
		ASSETS_ACTION_MENU_ROWS[1].disabled	  = _selected_folder_node.is_null();
		ASSETS_ACTION_MENU_ROWS[2].disabled	  = !folder_actions;
		ASSETS_ACTION_MENU_ROWS[3].disabled	  = !folder_actions;
		ASSETS_ACTION_MENU_ROWS[4].disabled	  = !folder_actions || multi_selected;
		ASSETS_ACTION_MENU_ROWS[5].disabled	  = !folder_actions;
		ASSETS_ACTION_MENU_ROWS[6].disabled	  = !folder_context || multi_selected;
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

		_create_asset_popup_command				 = 0;
		_create_folder_popup_pending			 = false;
		const editor_asset_tree_t& tree			 = editor_asset_manager_t::get().get_asset_tree();
		const bool				   is_asset_node = !_selected_asset_node.is_null() && tree.is_valid(_selected_asset_node) && tree.value(_selected_asset_node).type == editor_asset_node_type_e::asset;
		const bool				   is_file_node =
			!_selected_asset_node.is_null() && tree.is_valid(_selected_asset_node) && (tree.value(_selected_asset_node).type == editor_asset_node_type_e::file || tree.value(_selected_asset_node).type == editor_asset_node_type_e::script_file);
		const bool			  multi_selected	   = _selected_asset_nodes.size() > 1;
		const editor_asset_t* selected_asset	   = is_asset_node ? editor_asset_manager_t::get().find_asset(tree.value(_selected_asset_node).asset_id) : nullptr;
		ASSETS_ITEM_ACTION_MENU_ROWS[0].disabled   = multi_selected;
		ASSETS_ITEM_ACTION_MENU_ROWS[1].disabled   = multi_selected || selected_asset == nullptr || selected_asset->status == editor_asset_status_e::ok;
		ASSETS_ITEM_ACTION_MENU_ROWS[2].disabled   = !is_asset_node;
		ASSETS_ITEM_ACTION_MENU_ROWS[4].disabled   = multi_selected;
		ASSETS_ITEM_ACTION_MENU_ROWS[5].disabled   = is_file_node;
		ASSETS_ITEM_ACTION_MENU_ROWS[5].icon_color = editor_theme_t::get().color_accent1;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = ASSETS_ITEM_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(ASSETS_ITEM_ACTION_MENU_ROWS) / sizeof(ASSETS_ITEM_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_asset_action_menu_command;
		desc.command_user_data		   = this;
		desc.closed_fn				   = on_action_menu_closed;
		desc.closed_user_data		   = this;
		menu->request_action_menu(desc);
	}

}
