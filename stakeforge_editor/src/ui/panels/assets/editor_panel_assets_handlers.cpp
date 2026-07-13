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
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_manager_util.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "editor_surface_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/panels/assets/editor_panel_assets_internal.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include "assets/editor_asset_creator.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool editor_panel_assets_t::create_prefab_from_entity_payload(const editor_entity_payload_t& entity_payload, editor_asset_node_handle_t parent_node, bool allow_overwrite)
	{
		if (!editor_world_controller_t::get().is_world_valid(entity_payload.world))
			return false;

		world_t& world = editor_world_controller_t::get().get_editor_world(entity_payload.world)->get_world();
		if (entity_payload.entity == NULL_ENTITY_ID || !world.is_alive(entity_payload.entity))
			return false;

		nlohmann::json prefab_json = {};
		world_cooker_t::entity_to_json(world, entity_payload.entity, prefab_json, false);
		if (prefab_json.is_null())
			return false;

		const char* const				 entity_name = world.get_entity_name(entity_payload.entity);
		const string_t					 name		 = entity_name != nullptr && editor_directories_t::is_valid_asset_name(entity_name) ? entity_name : "prefab";
		const string_t					 json_text	 = prefab_json.dump();
		const editor_asset_create_desc_t desc{
			.parent_node	 = parent_node,
			.name			 = name.c_str(),
			.embedded_data	 = json_text.c_str(),
			.asset_type		 = editor_asset_type_e::prefab,
			.allow_overwrite = allow_overwrite,
		};
		editor_asset_t asset = {};
		if (!editor_asset_creator_t::create_asset(desc, &asset))
			return false;

		editor_asset_manager_t&			 asset_manager = editor_asset_manager_t::get();
		const string_t					 asset_path	   = editor_asset_path_t::make_asset_path(asset_manager.get_asset_tree().value(parent_node).full_path.c_str(), name.c_str());
		const editor_asset_node_handle_t existing	   = asset_manager.find_node_by_path(asset_path.c_str());
		if (!existing.is_null())
			asset_manager.reload_asset_node(existing);
		else
			asset_manager.add_path_node(parent_node, asset_path.c_str());
		world.make_prefab_chain(entity_payload.entity, asset.guid);
		world.refresh_prefab_instances(asset.guid, entity_payload.entity);
		editor_world_controller_t::get().mark_world_dirty(entity_payload.world);
		return true;
	}

	bool editor_panel_assets_t::create_prefabs_from_entity_payloads(span_t<const editor_entity_payload_t> entities, editor_asset_node_handle_t parent_node, bool allow_overwrite)
	{
		if (entities.data == nullptr || entities.size == 0)
			return false;

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!parent_node.is_null());
		SFG_ASSERT(tree.is_valid(parent_node));
		const editor_asset_node_t& parent = tree.value(parent_node);
		SFG_ASSERT(parent.type == editor_asset_node_type_e::folder);

		if (!allow_overwrite)
		{
			vector_t<string_t> rows;
			for (size_t i = 0; i < entities.size; ++i)
			{
				const editor_entity_payload_t& entity_payload = entities.data[i];
				if (!editor_world_controller_t::get().is_world_valid(entity_payload.world))
					continue;

				world_t& world = editor_world_controller_t::get().get_editor_world(entity_payload.world)->get_world();
				if (entity_payload.entity == NULL_ENTITY_ID || !world.is_alive(entity_payload.entity))
					continue;

				const char* const entity_name = world.get_entity_name(entity_payload.entity);
				const string_t	  name		  = entity_name != nullptr && editor_directories_t::is_valid_asset_name(entity_name) ? entity_name : "prefab";
				const string_t	  asset_path  = editor_asset_path_t::make_asset_path(parent.full_path.c_str(), name.c_str());
				string_t		  row;
				if (find_matching_asset_override(asset_path.c_str(), editor_asset_type_e::prefab, &row))
					rows.push_back(row);
			}

			if (!rows.empty())
			{
				_pending_override_target_folder = parent_node;
				_pending_override_entities.resize(0);
				_pending_override_entities.reserve(entities.size);
				for (size_t i = 0; i < entities.size; ++i)
					_pending_override_entities.push_back(entities.data[i]);
				request_assets_override(asset_override_operation_e::create_prefabs, "One or more prefabs already exist. Overwrite matching prefab assets?", rows);
				return true;
			}
		}

		bool created = false;
		for (size_t i = 0; i < entities.size; ++i)
			created = create_prefab_from_entity_payload(entities.data[i], parent_node, allow_overwrite) || created;
		return created;
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
		if (!file_system_t::exists(panel.get_selected_folder_path()))
			return;

		panel.clear_asset_grid_selection();
		vector_t<string_t> paths;
		process::select_files("Import Assets", ASSETS_IMPORT_FILE_EXTENSIONS, paths);
		if (!paths.empty())
			panel.import_assets(paths);
	}

	void editor_panel_assets_t::on_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);

		switch (command)
		{
		case assets_action_menu_create_folder:
			panel._create_folder_popup_pending = true;
			return;
		case assets_action_menu_create_world:
		case assets_action_menu_create_animation_state_machine:
		case assets_action_menu_create_opaque_shader:
		case assets_action_menu_create_transparent_shader:
		case assets_action_menu_create_post_process_shader:
		case assets_action_menu_create_ui_shader:
		case assets_action_menu_create_ui_text_shader:
		case assets_action_menu_create_texture_sampler:
		case assets_action_menu_create_gbuffer_material:
		case assets_action_menu_create_forward_material:
		case assets_action_menu_create_physical_material:
			panel._create_asset_popup_command = command;
			return;
		case assets_action_menu_import:
			on_import_button_pressed(false, &panel);
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
			if (!panel._selected_folder_hashes.empty())
			{
				const bool make_favourite = std::find(panel._favourite_folder_hashes.begin(), panel._favourite_folder_hashes.end(), panel._selected_folder_hashes.front()) == panel._favourite_folder_hashes.end();
				for (u64 hash : panel._selected_folder_hashes)
				{
					auto it = std::find(panel._favourite_folder_hashes.begin(), panel._favourite_folder_hashes.end(), hash);
					if (make_favourite && it == panel._favourite_folder_hashes.end())
						panel._favourite_folder_hashes.push_back(hash);
					else if (!make_favourite && it != panel._favourite_folder_hashes.end())
						panel._favourite_folder_hashes.erase(it);
				}
				panel.refresh_folder_rows();
			}
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
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		switch (command)
		{
		case assets_item_action_menu_rename:
			panel._asset_rename_popup_pending = true;
			return;
		case assets_item_action_menu_fix_integrity:
			panel.fix_asset_integrity();
			return;
		case assets_item_action_menu_duplicate:
			panel.duplicate_asset();
			return;
		case assets_item_action_menu_delete:
			panel.delete_asset();
			return;
		case assets_item_action_menu_open_directory: {
			const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
			if (panel._selected_asset_node.is_null() || !tree.is_valid(panel._selected_asset_node))
				return;

			const editor_asset_node_t& asset_node = tree.value(panel._selected_asset_node);
			if (asset_node.full_path.empty())
				return;

			const string_t directory = file_system_t::get_directory_of_file(asset_node.full_path.c_str());
			if (!directory.empty())
				process::open_directory(directory.c_str());
			return;
		}
		case assets_item_action_menu_toggle_favourite: {
			if (!panel._selected_asset_nodes.empty())
			{
				const sid_t first_guid	   = panel.get_asset_guid(panel._selected_asset_nodes.front());
				const bool	make_favourite = !panel.is_asset_favourite(first_guid);
				for (editor_asset_node_handle_t node : panel._selected_asset_nodes)
				{
					const sid_t guid = panel.get_asset_guid(node);
					if (guid == NULL_SID)
						continue;
					auto it = std::find(panel._favourite_asset_guids.begin(), panel._favourite_asset_guids.end(), guid);
					if (make_favourite && it == panel._favourite_asset_guids.end())
						panel._favourite_asset_guids.push_back(guid);
					else if (!make_favourite && it != panel._favourite_asset_guids.end())
						panel._favourite_asset_guids.erase(it);
				}
				if (panel._asset_favourites_only)
					panel.refresh_asset_grid(true);
				panel.refresh_asset_favourite_icons();
			}
			return;
		}
		default:
			return;
		}
	}

	void editor_panel_assets_t::on_action_menu_closed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (panel._create_asset_popup_command != 0)
		{
			panel.open_create_asset_popup(panel._create_asset_popup_command);
			return;
		}

		if (panel._create_folder_popup_pending)
		{
			panel._create_folder_popup_pending = false;
			panel.open_create_folder_popup();
			return;
		}

		if (panel._rename_popup_pending)
		{
			panel._rename_popup_pending = false;
			panel.open_rename_popup();
			return;
		}

		if (panel._asset_rename_popup_pending)
		{
			panel._asset_rename_popup_pending = false;
			panel.open_asset_rename_popup();
		}
	}

	void editor_panel_assets_t::on_create_folder_popup_closed(const char* value, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->create_folder(value);
	}

	void editor_panel_assets_t::on_create_asset_popup_closed(const char* value, void* user_data)
	{
		editor_panel_assets_t& panel	  = *static_cast<editor_panel_assets_t*>(user_data);
		const u16			   command	  = panel._create_asset_popup_command;
		panel._create_asset_popup_command = 0;
		panel.create_asset_item(command, value);
	}

	void editor_panel_assets_t::on_rename_popup_closed(const char* value, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->rename_folder(value);
	}

	void editor_panel_assets_t::on_asset_rename_popup_closed(const char* value, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->rename_asset_item(value);
	}

	void editor_panel_assets_t::on_cook_options_imported(void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->submit_pending_import();
	}

	void editor_panel_assets_t::on_cook_options_cancelled(void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->clear_pending_import();
	}

	void editor_panel_assets_t::on_search_changed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._search_str_lower = panel._search_str;
		string_util::to_lower(panel._search_str_lower);
		panel.refresh_folder_rows();
	}

	void editor_panel_assets_t::on_asset_search_changed(void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._asset_search_str_lower = panel._asset_search_str;
		string_util::to_lower(panel._asset_search_str_lower);
		panel.refresh_asset_grid(true);
	}

	void editor_panel_assets_t::on_show_file_assets_pressed(bool toggled, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._show_file_assets = toggled;
		panel.refresh_asset_grid(true);
	}

	void editor_panel_assets_t::on_asset_favourites_only_pressed(bool toggled, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel._asset_favourites_only = toggled;
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

	void editor_panel_assets_t::on_asset_tree_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press)
			return;

		editor_panel_assets_t&	   panel		   = *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_tree_t& tree			   = editor_asset_manager_t::get().get_asset_tree();
		const bool				   asset_selected  = !panel._selected_asset_node.is_null() && tree.is_valid(panel._selected_asset_node);
		const bool				   folder_selected = !panel._selected_folder_node.is_null() && panel._selected_folder_hash != 0 && tree.is_valid(panel._selected_folder_node) && !(panel._selected_folder_node == editor_asset_manager_t::get().get_root_node());
		const bool				   ctrl_pressed	   = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (ev.key == static_cast<u16>(input_code::key_a) && ctrl_pressed)
		{
			if (panel.is_focus_in_folder_pane())
				panel.select_all_visible_folders();
			else if (panel.is_focus_in_asset_items())
				panel.select_all_visible_assets();
		}
		else if (ev.key == static_cast<u16>(input_code::key_delete))
		{
			if (asset_selected)
				panel.delete_asset();
			else if (folder_selected)
				panel.delete_folder();
		}
		else if (ev.key == static_cast<u16>(input_code::key_d) && ctrl_pressed)
		{
			if (asset_selected)
				panel.duplicate_asset();
			else if (folder_selected)
				panel.duplicate_folder();
		}
		else if (ev.key == static_cast<u16>(input_code::key_f2))
		{
			if (asset_selected && panel._selected_asset_nodes.size() <= 1)
				panel.open_asset_rename_popup();
			else if (folder_selected && panel._selected_folder_hashes.size() <= 1)
				panel.open_rename_popup();
		}
	}

	void editor_panel_assets_t::on_assets_focus_gain(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->set_focus_state(true);
	}

	void editor_panel_assets_t::on_assets_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_assets_t::on_asset_item_focus_gain(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.set_focus_state(true);
		if (from_nav)
		{
			const asset_grid_item_t* const item = panel.find_asset_grid_item_by_widget(id);
			if (item != nullptr)
				panel.select_asset_grid_item(item->node, false, false);
		}
	}

	void editor_panel_assets_t::on_asset_item_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_assets_t::on_folder_row_focus_gain(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.set_focus_state(true);
		if (from_nav)
		{
			const folder_row_t* const row = panel.find_row_by_widget(id, false);
			if (row != nullptr)
				panel.select_folder_row(row->node, row->path_hash, false, false);
		}
	}

	void editor_panel_assets_t::on_folder_row_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_assets_t::on_assets_body_wheel(ui::input_router_t&, ui::widget_id_t id, f32 delta, void* user_data)
	{
		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		if (id == panel._assets_body_pane_top)
			panel._right_scrollbar.scroll_y(delta);
		else
			panel._left_scrollbar.scroll_y(delta);
	}

	bool editor_panel_assets_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::asset && payload.type != editor_payload_type_e::asset_multi && payload.type != editor_payload_type_e::folder && payload.type != editor_payload_type_e::folder_multi &&
			payload.type != editor_payload_type_e::entity && payload.type != editor_payload_type_e::entity_multi)
			return false;
		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_panel_assets_t&	   panel = *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_tree_t& tree	 = editor_asset_manager_t::get().get_asset_tree();
		const vec2f_t			   mouse = panel._ui->get_input().get_mouse_position();
		const folder_row_t* const  row	 = panel.find_row_by_pos(mouse);

		editor_asset_node_handle_t target_folder  = row != nullptr ? row->node : editor_asset_node_handle_t{};
		const bool				   entity_payload = payload.type == editor_payload_type_e::entity || payload.type == editor_payload_type_e::entity_multi;
		if (target_folder.is_null() && entity_payload)
		{
			const ui::layout_out_t& body_out = panel._ui->get_tree().out(panel._assets_body_pane_top);
			if (rectf_t{body_out.pos.x, body_out.pos.y, body_out.size.x, body_out.size.y}.contains(mouse))
				target_folder = panel._selected_folder_node;
		}

		if (target_folder.is_null() || !tree.is_valid(target_folder))
			return false;

		if (tree.value(target_folder).type != editor_asset_node_type_e::folder)
			return false;

		bool moved			= false;
		bool prefab_created = false;
		if (payload.type == editor_payload_type_e::entity)
		{
			const editor_entity_payload_t& entity = *static_cast<const editor_entity_payload_t*>(payload.user_ptr);
			moved								  = panel.create_prefabs_from_entity_payloads({.data = &entity, .size = 1}, target_folder, false);
			if (panel._pending_override_operation == asset_override_operation_e::create_prefabs)
				return true;
			prefab_created = moved;
		}
		else if (payload.type == editor_payload_type_e::entity_multi)
		{
			const vector_t<editor_entity_payload_t>& entities = *static_cast<const vector_t<editor_entity_payload_t>*>(payload.user_ptr);
			moved											  = panel.create_prefabs_from_entity_payloads({.data = entities.data(), .size = entities.size()}, target_folder, false);
			if (panel._pending_override_operation == asset_override_operation_e::create_prefabs)
				return true;
			prefab_created = moved;
		}
		else if (payload.type == editor_payload_type_e::folder)
		{
			const editor_asset_node_handle_t payload_node = *static_cast<editor_asset_node_handle_t*>(payload.user_ptr);
			if (payload_node.is_null() || !tree.is_valid(payload_node))
				return false;

			const editor_asset_node_t& node = tree.value(payload_node);
			if (node.type != editor_asset_node_type_e::folder)
				return false;

			panel._payload_folder_nodes.resize(0);
			panel._payload_folder_nodes.push_back(payload_node);
			moved = panel.move_payload_folders(panel._payload_folder_nodes, target_folder);
		}
		else if (payload.type == editor_payload_type_e::folder_multi)
		{
			const vector_t<editor_asset_node_handle_t>& nodes = *static_cast<const vector_t<editor_asset_node_handle_t>*>(payload.user_ptr);
			moved											  = panel.move_payload_folders(nodes, target_folder);
		}
		else if (payload.type == editor_payload_type_e::asset)
		{
			const editor_asset_node_handle_t payload_node = *static_cast<editor_asset_node_handle_t*>(payload.user_ptr);
			if (payload_node.is_null() || !tree.is_valid(payload_node))
				return false;

			const editor_asset_node_t& node = tree.value(payload_node);
			if (node.type != editor_asset_node_type_e::asset)
				return false;

			panel._payload_asset_nodes.resize(0);
			panel._payload_asset_nodes.push_back(payload_node);
			moved = panel.move_payload_assets(panel._payload_asset_nodes, target_folder);
			if (panel._pending_override_operation == asset_override_operation_e::move_assets)
				return true;
		}
		else
		{
			const vector_t<editor_asset_node_handle_t>& nodes = *static_cast<const vector_t<editor_asset_node_handle_t>*>(payload.user_ptr);
			moved											  = panel.move_payload_assets(nodes, target_folder);
			if (panel._pending_override_operation == asset_override_operation_e::move_assets)
				return true;
		}

		if (!moved)
			return false;

		panel.clear_asset_grid_selection();
		panel.refresh_folder_rows();
		if (prefab_created)
		{
			if (editor_panel_t* entities_panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::entities))
				static_cast<editor_panel_entities_t*>(entities_panel)->refresh_entities();
			if (editor_panel_t* inspector_panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::inspector))
				static_cast<editor_panel_inspector_t*>(inspector_panel)->refresh_from_selection();
		}
		return true;
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

	void editor_panel_assets_t::on_asset_grid_tick(ui::ui_context& ui, ui::widget_id_t id, f32, void* user_data)
	{
		editor_panel_assets_t&		  panel			= *static_cast<editor_panel_assets_t*>(user_data);
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();

		const bool rebuild_pending		  = panel._asset_grid_rebuild_pending;
		panel._asset_grid_rebuild_pending = false;

		const vec2f_t body_size = ui.get_tree().out(id).size;
		if (body_size.x > 0.0f && body_size.y > 0.0f)
		{
			if (!panel._asset_grid_body_size_valid)
			{
				panel._asset_grid_body_size_valid = true;
				panel._asset_grid_rebuild_pending = true;
			}
			else if (body_size != panel._asset_grid_body_size)
				panel._asset_grid_rebuild_pending = true;
			panel._asset_grid_body_size = body_size;
		}

		if (rebuild_pending)
			panel.refresh_asset_grid(true);
		else if (panel._asset_grid_generation != asset_manager.get_generation())
			panel.refresh_asset_grid(false);
	}

	void editor_panel_assets_t::on_asset_grid_scroll_restore_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->apply_pending_asset_grid_scroll_restore();
	}

	void editor_panel_assets_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_assets_t*>(user_data)->flush_pending_ui_mutations();
	}

	void editor_panel_assets_t::on_asset_grid_background_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		panel.open_action_menu(pos, false);
	}

	void editor_panel_assets_t::on_asset_grid_item_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t&		   panel = *static_cast<editor_panel_assets_t*>(user_data);
		const asset_grid_item_t* const item	 = panel.find_asset_grid_item_by_widget(id);
		if (item == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_asset_selected(item->node))
				panel.select_asset_grid_item(item->node, false, false);
			panel.open_asset_action_menu(pos);
		}
		else
		{
			const bool shift_pressed = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			const bool ctrl_pressed	 = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			panel.select_asset_grid_item(item->node, shift_pressed, ctrl_pressed);
		}
	}

	void editor_panel_assets_t::on_asset_grid_item_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_assets_t&		   panel = *static_cast<editor_panel_assets_t*>(user_data);
		const asset_grid_item_t* const item	 = panel.find_asset_grid_item_by_widget(id);
		if (item == nullptr)
			return;

		panel.open_asset_item(item->node);
	}

	void editor_panel_assets_t::on_asset_grid_item_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_assets_t&		   panel = *static_cast<editor_panel_assets_t*>(user_data);
		const asset_grid_item_t* const item	 = panel.find_asset_grid_item_by_widget(id);
		if (item == nullptr)
			return;

		panel.start_asset_item_payload(item->node);
	}

	void editor_panel_assets_t::on_folder_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_assets_t& panel = *static_cast<editor_panel_assets_t*>(user_data);
		panel.clear_asset_grid_selection();
		const folder_row_t* const row = panel.find_row_by_widget(id, true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_folder_selected(row->path_hash))
				panel.select_folder_row(row->node, row->path_hash, false, false);
			panel.open_action_menu(pos, true);
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
		const folder_row_t* const row = panel.find_row_by_widget(id, false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_folder_selected(row->path_hash))
				panel.select_folder_row(row->node, row->path_hash, false, false);
			panel.open_action_menu(pos, true);
		}
		else
		{
			const bool shift_pressed = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			const bool ctrl_pressed	 = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			panel.select_folder_row(row->node, row->path_hash, shift_pressed, ctrl_pressed);
		}
	}

	void editor_panel_assets_t::on_folder_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_assets_t&	  panel = *static_cast<editor_panel_assets_t*>(user_data);
		const folder_row_t* const row	= panel.find_row_by_widget(id, false);
		if (row == nullptr)
			return;

		panel.start_folder_payload(row->node);
	}

	void editor_panel_assets_t::on_folder_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_assets_t&	  panel = *static_cast<editor_panel_assets_t*>(user_data);
		const folder_row_t* const row	= panel.find_row_by_widget(id, false);
		if (row == nullptr || !row->has_children)
			return;
		panel.toggle_folder_fold(row->path_hash);
	}
}
