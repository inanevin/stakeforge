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
#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "assets/editor_asset_creator.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_assets_t::import_assets(const vector_t<string_t>& paths)
	{
		clear_pending_import();
		collect_pending_import_options(paths);
		if (_pending_import_options.empty())
			return;

		frame_vector_t<editor_modal_cook_option_desc_t> modal_options;
		modal_options.reserve(_pending_import_options.size());
		for (editor_asset_import_options_t& option : _pending_import_options)
		{
			switch (option.type)
			{
			case editor_asset_import_type_e::texture:
				modal_options.push_back({.object = &option.texture_cook_config, .title = "Texture", .type_id = type_id_t<texture_cook_config_t>::value});
				break;
			case editor_asset_import_type_e::audio:
				modal_options.push_back({.object = &option.audio_cook_config, .title = "Audio", .type_id = type_id_t<audio_cook_config_t>::value});
				break;
			case editor_asset_import_type_e::model:
				modal_options.push_back({.object = &option.glb_cook_config, .title = "Model", .type_id = type_id_t<glb_cook_config_t>::value});
				break;
			case editor_asset_import_type_e::hdr_skybox:
				modal_options.push_back({.object = &option.skybox_cook_config, .title = "HDR Skybox", .type_id = type_id_t<skybox_hdr_cook_config_t>::value});
				break;
			default:
				break;
			}
		}
		_cook_options_modal.set_options(modal_options.data(), static_cast<u16>(modal_options.size()));

		editor_modal_button_desc_t buttons[] = {
			{.text = "Cancel", .callback = on_cook_options_cancelled, .user_data = this},
			{.text = "Import", .callback = on_cook_options_imported, .user_data = this},
		};

		editor_modal_controller_t* modal = editor_modal_controller_t::find(*_ui);
		SFG_ASSERT(modal != nullptr);
		editor_modal_content_desc_t cook_options_content = _cook_options_modal.get_content_desc();
		modal->request_modal("Cook Options", "Configure cook options for the imported assets.", true, buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), &cook_options_content);
	}

	void editor_panel_assets_t::collect_pending_import_options(const vector_t<string_t>& paths)
	{
		_pending_import_paths.reserve(paths.size());
		_pending_import_options.reserve(4);
		for (const string_t& path : paths)
		{
			if (_pending_import_paths.size() == ASSETS_IMPORT_FILE_MAX)
				break;

			editor_asset_import_options_t option = {};
			if (!editor_asset_importer_t::make_import_options(option, path.c_str()))
				continue;

			_pending_import_paths.push_back(path);
			const auto option_it = std::find_if(_pending_import_options.begin(), _pending_import_options.end(), [&](const editor_asset_import_options_t& pending_option) { return pending_option.type == option.type; });
			if (option_it == _pending_import_options.end())
				_pending_import_options.push_back(option);
		}
	}

	void editor_panel_assets_t::submit_pending_import()
	{
		frame_vector_t<string_t> import_paths;
		import_paths.reserve(_pending_import_paths.size());
		for (const string_t& path : _pending_import_paths)
			import_paths.push_back(path);

		frame_vector_t<editor_asset_import_options_t> import_options;
		import_options.reserve(_pending_import_options.size());
		for (const editor_asset_import_options_t& option : _pending_import_options)
			import_options.push_back(option);

		editor_asset_manager_t::get().import_assets(_selected_folder_node, import_paths, import_options);
		clear_pending_import();
	}

	void editor_panel_assets_t::clear_pending_import()
	{
		_pending_import_paths.resize(0);
		_pending_import_options.resize(0);
	}

	void editor_panel_assets_t::delete_folder()
	{
		SFG_ASSERT(!_selected_folder_hashes.empty());

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		for (u64 folder_hash : _selected_folder_hashes)
		{
			const folder_row_t* row = find_row_by_hash(folder_hash);
			if (row == nullptr || row->node.is_null() || !tree.is_valid(row->node))
				continue;
			if (!editor_asset_util_t::delete_folder(row->node))
				continue;
			if (auto it = std::find(_favourite_folder_hashes.begin(), _favourite_folder_hashes.end(), folder_hash); it != _favourite_folder_hashes.end())
				_favourite_folder_hashes.erase(it);
			if (auto it = std::find(_expanded_folder_hashes.begin(), _expanded_folder_hashes.end(), folder_hash); it != _expanded_folder_hashes.end())
				_expanded_folder_hashes.erase(it);
		}

		clear_folder_selection();

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::duplicate_folder()
	{
		SFG_ASSERT(!_selected_folder_hashes.empty());

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		string_t				   last_duplicate_path;
		for (u64 folder_hash : _selected_folder_hashes)
		{
			const folder_row_t* row = find_row_by_hash(folder_hash);
			if (row != nullptr && !row->node.is_null() && tree.is_valid(row->node))
				editor_asset_util_t::duplicate_folder(row->node, &last_duplicate_path);
		}

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
		if (!last_duplicate_path.empty())
			select_folder_by_full_path(last_duplicate_path.c_str());
	}

	void editor_panel_assets_t::delete_asset()
	{
		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		if (_selected_asset_nodes.empty())
			return;

		string_t last_duplicate_path;
		for (editor_asset_node_handle_t node : _selected_asset_nodes)
		{
			if (node.is_null() || !tree.is_valid(node))
				continue;

			const editor_asset_node_t& asset_node = tree.value(node);
			if (asset_node.type == editor_asset_node_type_e::asset)
			{
				const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
				SFG_ASSERT(asset != nullptr);

				const sid_t guid = asset->guid;
				if (!editor_asset_util_t::delete_asset(*asset, node))
					continue;

				if (auto it = std::find(_favourite_asset_guids.begin(), _favourite_asset_guids.end(), guid); it != _favourite_asset_guids.end())
					_favourite_asset_guids.erase(it);
			}
			else if (asset_node.type == editor_asset_node_type_e::file)
				editor_asset_util_t::delete_file(node);
		}

		_selected_asset_node = {};
		_selected_asset_nodes.resize(0);
		_asset_selection_anchor = {};
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::duplicate_asset()
	{
		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		if (_selected_asset_nodes.empty())
			return;

		string_t last_duplicate_path;
		for (editor_asset_node_handle_t node : _selected_asset_nodes)
		{
			SFG_ASSERT(tree.is_valid(node));
			const editor_asset_node_t& asset_node = tree.value(node);
			if (asset_node.type != editor_asset_node_type_e::asset)
				continue;

			const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
			SFG_ASSERT(asset != nullptr);

			editor_asset_util_t::duplicate_asset(*asset, node, &last_duplicate_path);
		}

		_selected_asset_node = {};
		_selected_asset_nodes.resize(0);
		_asset_selection_anchor = {};
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
		if (!last_duplicate_path.empty())
			select_asset_by_full_path(last_duplicate_path.c_str());
	}

	void editor_panel_assets_t::open_asset_item(editor_asset_node_handle_t node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(tree.is_valid(node));

		const editor_asset_node_t& asset_node = tree.value(node);
		const editor_asset_t*	   asset	  = editor_asset_manager_t::get().find_asset(asset_node.asset_id);
		SFG_ASSERT(asset != nullptr);

		if (asset->asset_type == editor_asset_type_e::world)
			editor_app_t::get().get_world_controller().load_main_world(asset->guid);
	}

	void editor_panel_assets_t::fix_asset_integrity()
	{
		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		SFG_ASSERT(tree.is_valid(_selected_asset_node));

		const editor_asset_node_t& asset_node = tree.value(_selected_asset_node);
		if (asset_node.type != editor_asset_node_type_e::asset)
			return;

		const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
		SFG_ASSERT(asset != nullptr);

		if (asset->status == editor_asset_status_e::ok)
			return;

		if (asset->status != editor_asset_status_e::missing_file_source)
			return;

		const string_t selected_file = process::select_file("Fix Integrity", ASSETS_FIX_INTEGRITY_FILE_EXTENSIONS);
		if (selected_file.empty())
			return;

		const string_t assets_path	   = editor_project_t::get()._runtime.assets_path;
		const string_t source_relative = editor_asset_util_t::get_source_relative(assets_path.c_str(), selected_file.c_str());
		if (source_relative.empty())
		{
			SFG_ERR("selected source file is not relative to assets directory {0}", selected_file.c_str());
			return;
		}

		editor_asset_t fixed_asset	= *asset;
		fixed_asset.source_relative = source_relative;
		if (!editor_asset_util_t::write_asset(asset_node.full_path.c_str(), fixed_asset))
			return;

		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		asset_manager.ensure_integrity();
		refresh_folder_rows();
	}

	void editor_panel_assets_t::open_create_folder_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_create_folder_popup_closed;
		desc.user_data				   = this;
		desc.text					   = "folder";
		desc.placeholder			   = "Folder Name";
		desc.pos					   = _action_menu_pos;
		desc.width					   = editor_theme_t::get().item_width;
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::create_folder(const char* name)
	{
		string_t folder_name = name != nullptr ? name : "";
		if (!editor_directories_t::is_valid_asset_name(folder_name.c_str()))
			return;

		const string_t parent_path = get_action_menu_target_folder_path();
		if (parent_path.empty())
			return;

		string_t folder_path = editor_asset_util_t::normalize_directory(parent_path.c_str());
		folder_path += folder_name;
		if (file_system_t::exists(folder_path.c_str()))
			return;

		if (!file_system_t::create_directory(folder_path.c_str()))
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::open_create_asset_popup(u16 command)
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const char* text = "";
		switch (command)
		{
		case assets_action_menu_create_world:
			text = "world";
			break;
		case assets_action_menu_create_animation_state_machine:
			text = "animation_state_machine";
			break;
		case assets_action_menu_create_opaque_shader:
			text = "opaque_shader";
			break;
		case assets_action_menu_create_transparent_shader:
			text = "transparent_shader";
			break;
		case assets_action_menu_create_post_process_shader:
			text = "post_process_shader";
			break;
		case assets_action_menu_create_ui_shader:
			text = "ui_shader";
			break;
		case assets_action_menu_create_ui_text_shader:
			text = "ui_text_shader";
			break;
		case assets_action_menu_create_texture_sampler:
			text = "texture_sampler";
			break;
		case assets_action_menu_create_gbuffer_material:
			text = "gbuffer_material";
			break;
		case assets_action_menu_create_forward_material:
			text = "forward_material";
			break;
		case assets_action_menu_create_physical_material:
			text = "physical_material";
			break;
		default:
			SFG_ASSERT(false);
			return;
		}

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_create_asset_popup_closed;
		desc.user_data				   = this;
		desc.text					   = text;
		desc.placeholder			   = "Asset Name";
		desc.pos					   = _action_menu_pos;
		desc.width					   = editor_theme_t::get().item_width;
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::create_asset_item(u16 command, const char* name)
	{
		string_t asset_name = name != nullptr ? name : "";

		editor_asset_create_desc_t desc = {
			.parent_node	 = _selected_folder_node,
			.name			 = asset_name.c_str(),
			.allow_overwrite = true,
		};

		switch (command)
		{
		case assets_action_menu_create_world:
			desc.asset_type = editor_asset_type_e::world;
			break;
		case assets_action_menu_create_animation_state_machine:
			desc.asset_type = editor_asset_type_e::animation_state_machine;
			break;
		case assets_action_menu_create_opaque_shader:
			desc.asset_type = editor_asset_type_e::shader;
			desc.sub_type	= static_cast<u8>(shader_type_e::opaque_shader);
			break;
		case assets_action_menu_create_transparent_shader:
			desc.asset_type = editor_asset_type_e::shader;
			desc.sub_type	= static_cast<u8>(shader_type_e::transparent_shader);
			break;
		case assets_action_menu_create_post_process_shader:
			desc.asset_type = editor_asset_type_e::shader;
			desc.sub_type	= static_cast<u8>(shader_type_e::post_process_shader);
			break;
		case assets_action_menu_create_ui_shader:
			desc.asset_type = editor_asset_type_e::shader;
			desc.sub_type	= static_cast<u8>(shader_type_e::ui_shader);
			break;
		case assets_action_menu_create_ui_text_shader:
			desc.asset_type = editor_asset_type_e::shader;
			desc.sub_type	= static_cast<u8>(shader_type_e::ui_text_shader);
			break;
		case assets_action_menu_create_texture_sampler:
			desc.asset_type = editor_asset_type_e::texture_sampler;
			break;
		case assets_action_menu_create_gbuffer_material:
			desc.asset_type = editor_asset_type_e::material;
			desc.sub_type	= static_cast<u8>(editor_material_type_e::gbuffer);
			break;
		case assets_action_menu_create_forward_material:
			desc.asset_type = editor_asset_type_e::material;
			desc.sub_type	= static_cast<u8>(editor_material_type_e::forward);
			break;
		case assets_action_menu_create_physical_material:
			desc.asset_type = editor_asset_type_e::physical_material;
			break;
		default:
			SFG_ASSERT(false);
			return;
		}

		if (!editor_asset_creator_t::create_asset(desc))
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

	void editor_panel_assets_t::open_rename_popup()
	{
		SFG_ASSERT(!_selected_folder_node.is_null());
		SFG_ASSERT(_selected_folder_hash != 0);

		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(tree.is_valid(_selected_folder_node));

		const folder_row_t* target_row = find_row_by_hash(_selected_folder_hash);
		SFG_ASSERT(target_row != nullptr);

		const ui::layout_out_t& row_out = _ui->get_tree().out(target_row->root);
		const f32				scale	= ui::get_valid_scale(_ui->get_ui_scale());

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_rename_popup_closed;
		desc.user_data				   = this;
		desc.text					   = tree.value(_selected_folder_node).name.c_str();
		desc.pos					   = row_out.pos;
		desc.width					   = math::max(row_out.size.x / scale, editor_theme_t::get().item_width);
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::rename_folder(const char* name)
	{
		SFG_ASSERT(!_selected_folder_node.is_null());
		SFG_ASSERT(_selected_folder_hash != 0);

		string_t new_name = name != nullptr ? name : "";
		if (!editor_directories_t::is_valid_asset_name(new_name.c_str()))
			return;

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(tree.is_valid(_selected_folder_node));

		const string_t& old_name = tree.value(_selected_folder_node).name;
		if (new_name == old_name)
			return;

		const string_t& old_path = tree.value(_selected_folder_node).full_path;
		SFG_ASSERT(!old_path.empty());

		const string_t parent_path = file_system_t::get_directory_of_file(old_path.c_str());
		SFG_ASSERT(!parent_path.empty());

		const string_t new_path = parent_path + new_name;
		if (file_system_t::exists(new_path.c_str()))
			return;

		const u64 old_hash = _selected_folder_hash;
		const u64 new_hash = get_folder_hash_after_rename(_selected_folder_node, new_name);
		if (!editor_asset_util_t::rename_folder(_selected_folder_node, new_path.c_str()))
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

	void editor_panel_assets_t::open_asset_rename_popup()
	{
		SFG_ASSERT(!_selected_asset_node.is_null());

		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(tree.is_valid(_selected_asset_node));

		const editor_asset_node_t& asset_node = tree.value(_selected_asset_node);
		SFG_ASSERT(asset_node.type == editor_asset_node_type_e::asset || asset_node.type == editor_asset_node_type_e::file);

		const auto item_it = std::find_if(_asset_grid_items.begin(), _asset_grid_items.end(), [&](const asset_grid_item_t& item) { return item.node == _selected_asset_node; });
		SFG_ASSERT(item_it != _asset_grid_items.end());

		const ui::layout_out_t& item_out = _ui->get_tree().out(item_it->root);
		const f32				scale	 = ui::get_valid_scale(_ui->get_ui_scale());

		editor_input_popup_desc_t desc = {};
		desc.closed					   = on_asset_rename_popup_closed;
		desc.user_data				   = this;
		const string_t file_name_stem  = asset_node.type == editor_asset_node_type_e::file ? file_system_t::remove_extensions_from_path(asset_node.name) : "";
		desc.text					   = asset_node.type == editor_asset_node_type_e::file ? file_name_stem.c_str() : asset_node.name.c_str();
		desc.pos					   = item_out.pos;
		desc.width					   = math::max(item_out.size.x / scale, editor_theme_t::get().item_width);
		popup->request_input_popup(desc);
	}

	void editor_panel_assets_t::rename_asset_item(const char* name)
	{
		editor_asset_manager_t&	   asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t& tree			 = asset_manager.get_asset_tree();
		SFG_ASSERT(tree.is_valid(_selected_asset_node));

		string_t new_name = name != nullptr ? name : "";
		if (!editor_directories_t::is_valid_asset_name(new_name.c_str()))
			return;

		const editor_asset_node_t& asset_node = tree.value(_selected_asset_node);
		SFG_ASSERT(asset_node.type == editor_asset_node_type_e::asset || asset_node.type == editor_asset_node_type_e::file);

		const string_t old_file_extension = asset_node.type == editor_asset_node_type_e::file ? file_system_t::get_file_extension(asset_node.name) : "";
		if (asset_node.type == editor_asset_node_type_e::file)
		{
			new_name = file_system_t::remove_extensions_from_path(new_name);
			if (!editor_directories_t::is_valid_asset_name(new_name.c_str()))
				return;
		}

		if (new_name == asset_node.name)
			return;

		const string_t& old_path = asset_node.full_path;
		SFG_ASSERT(!old_path.empty());

		const string_t parent_path = file_system_t::get_directory_of_file(old_path.c_str());
		SFG_ASSERT(!parent_path.empty());

		string_t new_path = parent_path + new_name;
		if (asset_node.type == editor_asset_node_type_e::asset)
			new_path += ".sfg_asset";
		else if (!old_file_extension.empty())
		{
			new_path += ".";
			new_path += old_file_extension;
		}
		if (file_system_t::exists(new_path.c_str()))
			return;

		if (asset_node.type == editor_asset_node_type_e::asset)
		{
			const editor_asset_t* asset = asset_manager.find_asset(asset_node.asset_id);
			SFG_ASSERT(asset != nullptr);

			if (!editor_asset_util_t::rename_asset(*asset, _selected_asset_node, new_path.c_str()))
				return;
		}
		else if (!editor_asset_util_t::rename_file(_selected_asset_node, new_path.c_str()))
			return;

		_selected_asset_node = {};
		asset_manager.rescan(editor_project_t::get()._runtime.assets_path);
		refresh_folder_rows();
	}

}
