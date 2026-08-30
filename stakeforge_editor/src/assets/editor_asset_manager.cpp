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

#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager_util.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/thumbnail/editor_asset_thumbnail_manager.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "editor_app.hpp"
#include "editor_file_watch_controller.hpp"
#include "scripting/editor_script_manager.hpp"
#include "editor_surface_controller.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/frame_hash_map.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/color_utils.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader_data_definition.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define EDITOR_ASSET_COLOR(R, G, B)		   color_utils_t::srgb_to_linear(color_t::from255(R, G, B, 255.0f)).to_vector()
#define EDITOR_ASSET_COOK_BUCKET_CAPACITY  512
#define EDITOR_ASSET_DELETION_LISTENER_MAX 64
#define EDITOR_FILE_RECONCILE_MAX_PER_TICK 64

	namespace
	{
		material_def_t refresh_material_from_shader_definition(const material_def_t& source, const shader_data_definition_t& shader_definition)
		{

			material_def_t material	   = material_def_from_shader_def(shader_definition, source.shader);
			material.blend_mode		   = source.blend_mode;
			material.write_shadows	   = source.write_shadows;
			material.write_reflections = source.write_reflections;
			material.double_sided	   = source.double_sided;
			material.use_alpha_cutoff  = source.use_alpha_cutoff;

			for (material_texture_value_t& texture : material.textures)
			{
				const auto it = std::find_if(source.textures.begin(), source.textures.end(), [&](const material_texture_value_t& value) { return value.name == texture.name; });
				if (it != source.textures.end())
					texture.texture = it->texture;
			}

			for (material_sampler_value_t& sampler : material.samplers)
			{
				const auto it = std::find_if(source.samplers.begin(), source.samplers.end(), [&](const material_sampler_value_t& value) { return value.name == sampler.name; });
				if (it != source.samplers.end())
					sampler.sampler = it->sampler;
			}

			for (material_param_value_t& parameter : material.parameters)
			{
				const auto it = std::find_if(source.parameters.begin(), source.parameters.end(), [&](const material_param_value_t& value) { return value.name == parameter.name && value.type == parameter.type; });
				if (it == source.parameters.end())
					continue;

				parameter.hint = it->hint;
				for (u8 i = 0; i < 4; ++i)
				{
					if (parameter.type == shader_param_type_e::u32)
						parameter.value_u32[i] = it->value_u32[i];
					else
						parameter.value[i] = it->value[i];
				}
			}

			return material;
		}
	}

	bool editor_asset_manager_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);

		s_instance = this;

		_asset_deletion_listeners.init(EDITOR_ASSET_DELETION_LISTENER_MAX);
		_asset_cook_states.init(EDITOR_ASSET_COOK_BUCKET_CAPACITY);
		_asset_descriptors.clear();
		_asset_descriptors.reserve(static_cast<size_t>(editor_asset_type_e::count) - 1);

		register_descriptor({.extensions = {"mp3"}, .display_name = "Audio", .color = EDITOR_ASSET_COLOR(64.0f, 177.0f, 255.0f), .asset_type = editor_asset_type_e::audio});
		register_descriptor({.extensions = {"ttf"}, .display_name = "Font", .color = EDITOR_ASSET_COLOR(245.0f, 194.0f, 82.0f), .asset_type = editor_asset_type_e::font});
		register_descriptor({.extensions = {"glb"}, .display_name = "Mesh", .color = EDITOR_ASSET_COLOR(158.0f, 120.0f, 255.0f), .asset_type = editor_asset_type_e::mesh});
		register_descriptor({.display_name = "Skeleton", .color = EDITOR_ASSET_COLOR(184.0f, 155.0f, 255.0f), .asset_type = editor_asset_type_e::skeleton});
		register_descriptor({.display_name = "Animation", .color = EDITOR_ASSET_COLOR(255.0f, 129.0f, 80.0f), .asset_type = editor_asset_type_e::animation});
		register_descriptor({.display_name = "Material", .color = EDITOR_ASSET_COLOR(255.0f, 102.0f, 0.0f), .asset_type = editor_asset_type_e::material});
		register_descriptor({.extensions = {"hlsl"}, .display_name = "Shader", .color = EDITOR_ASSET_COLOR(90.0f, 190.0f, 255.0f), .asset_type = editor_asset_type_e::shader});
		register_descriptor({.extensions = {"png", "jpg", "jpeg"}, .display_name = "Texture", .color = EDITOR_ASSET_COLOR(151.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture});
		register_descriptor({.display_name = "Texture Sampler", .color = EDITOR_ASSET_COLOR(180.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture_sampler});
		register_descriptor({.display_name = "Physical Material", .color = EDITOR_ASSET_COLOR(214.0f, 65.0f, 57.0f), .asset_type = editor_asset_type_e::physical_material});
		register_descriptor({.display_name = "Prefab", .color = EDITOR_ASSET_COLOR(107.0f, 210.0f, 132.0f), .asset_type = editor_asset_type_e::prefab});
		register_descriptor({.display_name = "Animation Graph", .color = EDITOR_ASSET_COLOR(245.0f, 118.0f, 182.0f), .asset_type = editor_asset_type_e::animation_graph});
		register_descriptor({.extensions = {"hdr"}, .display_name = "Cubemap", .color = EDITOR_ASSET_COLOR(87.0f, 175.0f, 142.0f), .asset_type = editor_asset_type_e::cubemap});
		register_descriptor({.display_name = "Physics Collision Mesh", .color = EDITOR_ASSET_COLOR(214.0f, 96.0f, 57.0f), .asset_type = editor_asset_type_e::physics_collision_mesh});
		register_descriptor({.display_name = "Sprite", .color = EDITOR_ASSET_COLOR(212.0f, 85.0f, 169.0f), .asset_type = editor_asset_type_e::sprite});
		register_descriptor({.display_name = "Curve", .color = EDITOR_ASSET_COLOR(235.0f, 89.0f, 102.0f), .asset_type = editor_asset_type_e::curve});
		register_descriptor({.display_name = "Ragdoll", .color = EDITOR_ASSET_COLOR(184.0f, 155.0f, 255.0f), .asset_type = editor_asset_type_e::ragdoll});
		register_descriptor({.display_name = "World", .color = EDITOR_ASSET_COLOR(98.0f, 212.0f, 205.0f), .asset_type = editor_asset_type_e::world});

		clear();

		return true;
	}

	void editor_asset_manager_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		SFG_ASSERT(!_import_in_progress);
		SFG_ASSERT(_import_work.is_null());

		flush_asset_cook_jobs();

		clear();

		_import_state.target_directory.resize(0);
		_import_state.paths.resize(0);
		_import_state.imported_asset_paths.resize(0);
		_import_state.import_options.resize(0);
		_import_work_status_text.resize(0);
		_import_displayed_status.resize(0);
		_asset_descriptors.clear();
		_asset_deletion_listeners.uninit();
		_asset_cook_states.uninit();

		_last_asset_cook_work	  = {};
		_asset_cook_work_count	  = 0;
		_next_asset_cook_revision = 1;

		_file_reconcile_entries.resize(0);
		_file_reconcile_index		  = 0;
		_file_reconcile_root_mask	  = 0;
		_import_in_progress			  = false;
		_import_work				  = {};
		_import_work_progress		  = 0.0f;
		_cooked_file_track_inited	  = false;
		_source_file_track_inited	  = false;
		_script_file_track_inited	  = false;
		_script_compile_requested	  = false;
		_import_target_directory_node = {};

		s_instance = nullptr;
	}

	void editor_asset_manager_t::tick()
	{
		ZoneScoped;

		if (_import_in_progress)
		{
			SFG_ASSERT(!_import_work.is_null());

			editor_app_t::get().get_work_controller().get_work_status(_import_work, _import_work_progress, _import_work_status_text);

			editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

			_import_progress_modal.set_progress(_import_work_progress);

			if (_import_work_status_text != _import_displayed_status)
			{
				_import_displayed_status = _import_work_status_text;
				modal.set_body_text(_import_displayed_status.c_str());
			}
		}

		// integrity check.
		if (_last_integrity_generation != _generation && !_database.get_root_node().is_null())
		{
			editor_asset_manager_util_t::ensure_integrity(*this);
			_last_integrity_generation = _generation;
		}

		process_file_reconciliation();

		if (!_changed_cooked_resources.empty())
			process_changed_cooked_resources();

		if (!_changed_source_assets.empty())
			process_changed_source_files();

		if (_script_file_track_inited)
		{
			if (_script_compile_requested && editor_script_manager_t::get().is_compile_idle())
			{
				_script_compile_requested = false;
				editor_script_manager_t::get().compile_scripts();
			}
		}
	}

	void editor_asset_manager_t::clear()
	{
		SFG_ASSERT(!_import_in_progress);
		SFG_ASSERT(_import_work.is_null());
		SFG_ASSERT(_asset_cook_work_count == 0);
		SFG_ASSERT(_asset_cook_states.begin() == _asset_cook_states.end());

		_database.clear();
		_cooked_resource_tracking_states.clear();
		_cooked_path_to_resource.clear();
		_source_file_to_tracking.clear();
		_script_file_tracking_states.clear();
		_asset_to_source_tracking.clear();
		_changed_cooked_resources.resize(0);
		_changed_source_assets.resize(0);
		_file_reconcile_entries.resize(0);
		_latest_asset_cook_revisions.clear();
		_file_reconcile_index	  = 0;
		_file_reconcile_root_mask = 0;
		_cooked_file_track_inited = false;
		_source_file_track_inited = false;
		_script_file_track_inited = false;
		_script_compile_requested = false;
		_generation++;
	}

	void editor_asset_manager_t::initialize_cooked_resource_tracking()
	{
		_cooked_resource_tracking_states.clear();
		_cooked_resource_tracking_states.reserve(_database.get_assets().size() * 2);
		_cooked_path_to_resource.clear();
		_cooked_path_to_resource.reserve(_database.get_assets().size() * 2);
		_changed_cooked_resources.resize(0);
		_file_reconcile_entries.resize(0);
		_file_reconcile_entries.reserve(_database.get_assets().size() * 4);
		_file_reconcile_index	  = 0;
		_file_reconcile_root_mask = 0;

		for (const auto& asset_pair : _database.get_assets())
		{
			const editor_asset_t& asset = asset_pair.second;

			track_cooked_resource(asset.guid, asset.guid, cooked_resource_kind_e::asset, false);
			track_cooked_resource(asset.thumbnail_guid, asset.guid, cooked_resource_kind_e::thumbnail, false);

			if (editor_asset_thumbnailer_t::is_renderable_thumbnail(asset.asset_type) && asset.thumbnail_guid != editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset.asset_type))
			{
				const string_t thumbnail_cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);

				if (!file_system_t::exists(thumbnail_cache_path.c_str()))
					editor_asset_thumbnail_manager_t::get().request_render(asset.guid);
			}
		}

		_cooked_file_track_inited = true;
	}

	void editor_asset_manager_t::initialize_source_file_tracking()
	{
		_source_file_to_tracking.clear();
		_source_file_to_tracking.reserve(_database.get_assets().size());
		_asset_to_source_tracking.clear();
		_asset_to_source_tracking.reserve(_database.get_assets().size());
		_changed_source_assets.resize(0);

		for (const auto& asset_pair : _database.get_assets())
			track_source_asset(asset_pair.second);

		_source_file_track_inited = true;
	}

	void editor_asset_manager_t::initialize_script_file_tracking()
	{
		SFG_ASSERT(!_script_file_track_inited);

		const editor_asset_tree_t&		 tree = _database.get_asset_tree();
		const editor_asset_node_handle_t root = _database.get_root_node();

		SFG_ASSERT(tree.is_valid(root));

		_script_file_tracking_states.clear();
		_script_file_tracking_states.reserve(tree.size());

		tree.for_each_depth_first(root, [&](editor_asset_node_handle_t node, u32 depth) {
			const editor_asset_node_t& asset_node = tree.value(node);

			if (asset_node.type == editor_asset_node_type_e::script_file)
				track_script_file(asset_node.full_path.c_str());
		});

		_script_file_track_inited = true;
		_script_compile_requested = false;
	}

	void editor_asset_manager_t::uninitialize_script_file_tracking()
	{
		SFG_ASSERT(_script_file_track_inited);

		_script_file_tracking_states.clear();
		_script_file_track_inited = false;
		_script_compile_requested = false;
	}

	void editor_asset_manager_t::register_descriptor(const editor_asset_descriptor_t& desc)
	{
		_asset_descriptors[desc.asset_type] = desc;
	}

	editor_asset_node_handle_t editor_asset_manager_t::add_folder_node(editor_asset_node_handle_t parent, const char* path)
	{
		const string_t					 name  = file_system_t::get_last_folder_from_path(path);
		const u8						 flags = !name.empty() && name[0] == '_' ? editor_asset_node_flag_hidden : 0;
		const editor_asset_node_handle_t node  = _database.emplace_node(editor_asset_node_t{.name = name, .full_path = path, .type = editor_asset_node_type_e::folder, .flags = flags});
		_database.attach_node(parent, node);
		notify_changed();
		return node;
	}

	editor_asset_node_handle_t editor_asset_manager_t::add_path_node(editor_asset_node_handle_t parent, const char* path)
	{
		if (file_system_t::get_file_extension(path) == "sfg_asset")
		{
			editor_asset_t asset = {};
			if (!editor_asset_io_t::read_asset(path, asset))
				return {};

			if (asset.guid == NULL_SID)
			{
				SFG_ERR("asset {0} has an invalid guid", path);
				return {};
			}

			if (_database.find_asset(asset.guid) != nullptr)
			{
				SFG_ERR("asset {0} has a duplicate guid", path);
				return {};
			}

			const sid_t	   asset_id		= asset.guid;
			const sid_t	   thumbnail_id = asset.thumbnail_guid;
			const string_t name			= file_system_t::remove_extensions_from_path(file_system_t::get_filename_and_extension_from_path(path));

			_database.upsert_asset(std::move(asset));
			const editor_asset_node_handle_t node = _database.emplace_node(editor_asset_node_t{.asset_id = asset_id, .name = name, .full_path = path, .type = editor_asset_node_type_e::asset});
			_database.attach_node(parent, node);

			if (_cooked_file_track_inited)
			{
				track_cooked_resource(asset_id, asset_id, cooked_resource_kind_e::asset, true);
				track_cooked_resource(thumbnail_id, asset_id, cooked_resource_kind_e::thumbnail, true);
			}

			if (_source_file_track_inited)
				track_source_asset(*_database.find_asset(asset_id));

			notify_changed();
			return node;
		}

		const string_t					 name	   = file_system_t::get_filename_and_extension_from_path(path);
		const editor_asset_node_type_e	 node_type = file_system_t::get_file_extension(path) == "cs" ? editor_asset_node_type_e::script_file : editor_asset_node_type_e::file;
		const editor_asset_node_handle_t node	   = _database.emplace_node(editor_asset_node_t{.name = name, .full_path = path, .type = node_type});
		_database.attach_node(parent, node);

		if (_script_file_track_inited && node_type == editor_asset_node_type_e::script_file)
		{
			track_script_file(path);
			_script_compile_requested = true;
		}

		notify_changed();
		return node;
	}

	editor_asset_node_handle_t editor_asset_manager_t::add_directory_tree(editor_asset_node_handle_t parent, const char* path)
	{
		const editor_asset_node_handle_t directory = add_folder_node(parent, path);
		vector_t<file_system_entry_t>	 entries   = {};
		file_system_t::get_entries_recursive(path, entries);

		vector_t<string_t> parts = {};
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(path, entry.path.c_str());
			parts.resize(0);
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t current			 = directory;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				current = get_or_create_child_folder(current, parts[i]);

			if (entry.type == file_system_entry_type_e::file)
				add_path_node(current, entry.path.c_str());
		}

		return directory;
	}

	bool editor_asset_manager_t::reload_asset_node(editor_asset_node_handle_t node)
	{
		editor_asset_tree_t& tree		= _database.get_asset_tree();
		editor_asset_node_t& asset_node = tree.value(node);

		editor_asset_t asset = {};
		if (!editor_asset_io_t::read_asset(asset_node.full_path.c_str(), asset))
			return false;

		if (asset.guid == NULL_SID)
			return false;

		const sid_t old_guid	 = asset_node.asset_id;
		const sid_t asset_id	 = asset.guid;
		const sid_t thumbnail_id = asset.thumbnail_guid;

		if (_source_file_track_inited && old_guid != asset_id)
			untrack_source_asset(old_guid);

		_database.erase_asset(old_guid);
		asset_node.asset_id = asset_id;
		_database.upsert_asset(std::move(asset));
		_database.rebuild_indices();

		if (_cooked_file_track_inited)
		{
			track_cooked_resource(asset_id, asset_id, cooked_resource_kind_e::asset, true);
			track_cooked_resource(thumbnail_id, asset_id, cooked_resource_kind_e::thumbnail, true);
		}

		if (_source_file_track_inited)
			track_source_asset(*_database.find_asset(asset_id));

		notify_changed();
		return true;
	}

	void editor_asset_manager_t::sync_directory_from_disk(editor_asset_node_handle_t directory_node)
	{
		if (directory_node.is_null() || !_database.get_asset_tree().is_valid(directory_node))
			return;

		const editor_asset_tree_t& tree			  = _database.get_asset_tree();
		const editor_asset_node_t& directory	  = tree.value(directory_node);
		const string_t			   directory_path = directory.full_path;

		vector_t<file_system_entry_t> entries = {};
		file_system_t::get_entries_recursive(directory_path.c_str(), entries);

		vector_t<string_t> parts = {};
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(directory_path.c_str(), entry.path.c_str());
			parts.resize(0);
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t parent			 = directory_node;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				parent = get_or_create_child_folder(parent, parts[i]);

			if (entry.type == file_system_entry_type_e::directory)
				continue;

			const editor_asset_node_handle_t existing = _database.find_node_by_path(entry.path.c_str());
			if (existing.is_null())
				add_path_node(parent, entry.path.c_str());
			else if (file_system_t::get_file_extension(entry.path) == "sfg_asset")
				reload_asset_node(existing);
		}
	}

	void editor_asset_manager_t::sync_imported_asset_paths(editor_asset_node_handle_t directory_node, span_t<const string_t> paths)
	{
		if (directory_node.is_null() || !_database.get_asset_tree().is_valid(directory_node))
			return;

		const editor_asset_tree_t& tree			  = _database.get_asset_tree();
		const editor_asset_node_t& directory	  = tree.value(directory_node);
		const string_t			   directory_path = directory.full_path;

		vector_t<string_t> parts = {};
		for (size_t i = 0; i < paths.size; ++i)
		{
			const string_t&					 path	  = paths.data[i];
			const editor_asset_node_handle_t existing = _database.find_node_by_path(path.c_str());
			if (!existing.is_null())
			{
				reload_asset_node(existing);
				continue;
			}

			const string_t			   parent_path = file_system_t::get_directory_of_file(path.c_str());
			editor_asset_node_handle_t parent	   = _database.find_node_by_path(parent_path.c_str());
			if (parent.is_null())
			{
				const string_t relative = file_system_t::get_relative(directory_path.c_str(), parent_path.c_str());
				parent					= directory_node;
				if (!relative.empty() && relative != ".")
				{
					parts.resize(0);
					string_util::split(parts, relative, "/");
					for (const string_t& part : parts)
					{
						if (!part.empty() && part != ".")
							parent = get_or_create_child_folder(parent, part);
					}
				}
			}

			if (!parent.is_null())
				add_path_node(parent, path.c_str());
		}
	}

	bool editor_asset_manager_t::delete_node_subtree(editor_asset_node_handle_t node)
	{
		struct deleted_asset_t
		{
			sid_t				asset_guid	   = NULL_SID;
			sid_t				thumbnail_guid = NULL_SID;
			editor_asset_type_e asset_type	   = editor_asset_type_e::invalid;
		};

		const editor_asset_tree_t& tree = _database.get_asset_tree();
		SFG_ASSERT(tree.is_valid(node));
		SFG_ASSERT(node != _database.get_root_node());

		const editor_asset_node_t& root_node = tree.value(node);
		SFG_ASSERT((root_node.flags & editor_asset_node_flag_promoted) == 0);

		const string_t root_path   = root_node.full_path;
		const string_t assets_path = editor_project_t::get()._runtime.assets_path;

		// build the complete deletion set
		size_t deleted_asset_count = 0;

		tree.for_each_depth_first(node, [&](editor_asset_node_handle_t current, u32 depth) {
			if (tree.value(current).type == editor_asset_node_type_e::asset)
				deleted_asset_count++;
		});

		frame_vector_t<deleted_asset_t> deleted_assets = {};
		deleted_assets.reserve(deleted_asset_count);

		frame_vector_t<sid_t> deleted_asset_guids = {};
		deleted_asset_guids.reserve(deleted_asset_count);

		frame_vector_t<sid_t> deleted_script_ids = {};

		frame_hash_map_t<sid_t, bool> deleted_asset_ids = {};
		deleted_asset_ids.reserve(deleted_asset_count);

		frame_hash_map_t<u64, string_t> deleted_blob_paths = {};
		deleted_blob_paths.reserve(deleted_asset_count);

		tree.for_each_depth_first(node, [&](editor_asset_node_handle_t current, u32 depth) {
			const editor_asset_node_t& current_node = tree.value(current);

			if (current_node.type == editor_asset_node_type_e::script_file)
			{
				deleted_script_ids.push_back(editor_asset_path_t::hash_path(current_node.full_path.c_str()));
				return;
			}

			if (current_node.type != editor_asset_node_type_e::asset)
				return;

			const editor_asset_t* asset = _database.find_asset(current_node.asset_id);
			SFG_ASSERT(asset != nullptr);

			deleted_assets.push_back({
				.asset_guid		= asset->guid,
				.thumbnail_guid = asset->thumbnail_guid,
				.asset_type		= asset->asset_type,
			});
			deleted_asset_guids.push_back(asset->guid);
			deleted_asset_ids.emplace(asset->guid, true);

			if (asset->source_type == editor_asset_source_type_e::file_blob)
			{
				const string_t blob_path = editor_asset_path_t::get_source_full_path(assets_path.c_str(), *asset);
				deleted_blob_paths.try_emplace(editor_asset_path_t::hash_path(blob_path.c_str()), blob_path);
			}
		});

		// protect sources still used outside the subtree
		for (const auto& asset_pair : _database.get_assets())
		{
			const editor_asset_t& asset = asset_pair.second;

			if (deleted_asset_ids.find(asset.guid) != deleted_asset_ids.end() || asset.source_relative.empty())
				continue;

			const string_t source_path		 = editor_asset_path_t::get_source_full_path(assets_path.c_str(), asset);
			const bool	   source_is_deleted = root_node.type == editor_asset_node_type_e::folder ? editor_asset_path_t::is_path_in_directory(source_path.c_str(), root_path.c_str())
																								  : root_node.type == editor_asset_node_type_e::file && editor_asset_path_t::is_same_path(source_path.c_str(), root_path.c_str());

			if (source_is_deleted)
			{
				SFG_ERR("can't delete {0} because asset {1} uses source {2}", root_path, asset.guid, source_path);
				return false;
			}

			const auto blob_path = deleted_blob_paths.find(editor_asset_path_t::hash_path(source_path.c_str()));

			if (blob_path != deleted_blob_paths.end() && editor_asset_path_t::is_same_path(blob_path->second.c_str(), source_path.c_str()))
				deleted_blob_paths.erase(blob_path);
		}

		// finish pending writes before filesystem mutation
		flush_asset_cook_jobs();

		// delete the main path first
		const bool removed_from_disk = root_node.type == editor_asset_node_type_e::folder ? file_system_t::delete_directory(root_path.c_str()) : !file_system_t::exists(root_path.c_str()) || !file_system_t::delete_file(root_path.c_str());

		if (!removed_from_disk)
		{
			SFG_ERR("failed to delete {0}", root_path);
			return false;
		}

		// let consumers release deleted assets before force-unload
		if (!deleted_asset_guids.empty())
			notify_asset_deletion({.data = deleted_asset_guids.data(), .size = deleted_asset_guids.size()});

		// cancel thumbnail work and unload live resources
		editor_app_t::get().stop_render();

		editor_asset_thumbnail_manager_t& thumbnail_manager = editor_asset_thumbnail_manager_t::get();
		resource_manager_t&				  resource_manager	= resource_manager_t::get();

		for (const deleted_asset_t& asset : deleted_assets)
			thumbnail_manager.remove_asset(asset.asset_guid, asset.thumbnail_guid);

		for (const deleted_asset_t& asset : deleted_assets)
		{
			if (resource_manager.find_entry(asset.asset_guid) != nullptr)
			{
				resource_manager.unload_resource(asset.asset_guid, true);
				SFG_ASSERT(resource_manager.find_entry(asset.asset_guid) == nullptr);
			}

			untrack_cooked_resource(asset.asset_guid);
			untrack_cooked_resource(asset.thumbnail_guid);
			untrack_source_asset(asset.asset_guid);
		}

		// commit deletion to the asset database
		for (const sid_t script_id : deleted_script_ids)
			_script_file_tracking_states.erase(script_id);

		_database.remove_node_subtree(node);
		notify_changed();

		if (_script_file_track_inited && !deleted_script_ids.empty())
			_script_compile_requested = true;

		// remove cooked binaries and unowned blobs
		for (const deleted_asset_t& asset : deleted_assets)
		{
			const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.asset_guid);

			if (file_system_t::exists(cache_path.c_str()) && file_system_t::delete_file(cache_path.c_str()))
				SFG_ERR("failed to delete cooked asset {0}", cache_path);

			if (asset.thumbnail_guid == NULL_SID || asset.thumbnail_guid == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset.asset_type))
				continue;

			const string_t thumbnail_cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);

			if (file_system_t::exists(thumbnail_cache_path.c_str()) && file_system_t::delete_file(thumbnail_cache_path.c_str()))
				SFG_ERR("failed to delete thumbnail asset {0}", thumbnail_cache_path);
		}

		for (const auto& blob_pair : deleted_blob_paths)
		{
			const string_t& blob_path = blob_pair.second;

			if (file_system_t::exists(blob_path.c_str()) && file_system_t::delete_file(blob_path.c_str()))
				SFG_ERR("failed to delete asset blob {0}", blob_path);
		}

		return true;
	}

	editor_asset_deletion_listener_handle_t editor_asset_manager_t::add_asset_deletion_listener(editor_asset_deletion_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(fn != nullptr);

		const editor_asset_deletion_listener_handle_t handle   = _asset_deletion_listeners.emplace();
		editor_asset_deletion_listener_t&			  listener = _asset_deletion_listeners.get(handle);
		listener.fn											   = fn;
		listener.user_data									   = user_data;

		return handle;
	}

	void editor_asset_manager_t::remove_asset_deletion_listener(editor_asset_deletion_listener_handle_t handle)
	{
		if (_asset_deletion_listeners.is_valid(handle))
			_asset_deletion_listeners.remove(handle);
	}

	void editor_asset_manager_t::notify_asset_deletion(span_t<const sid_t> asset_ids)
	{
		for (auto it = _asset_deletion_listeners.begin_handle(); it != _asset_deletion_listeners.end_handle(); ++it)
		{
			const editor_asset_deletion_listener_handle_t handle   = *it;
			const editor_asset_deletion_listener_t&		  listener = _asset_deletion_listeners.get(handle);

			if (listener.fn != nullptr)
				listener.fn(*this, asset_ids, listener.user_data);
		}
	}

	void editor_asset_manager_t::update_node_path(editor_asset_node_handle_t node, const char* new_path)
	{
		const editor_asset_node_t&	   asset_node = _database.get_asset_tree().value(node);
		const string_t				   old_path	  = asset_node.full_path;
		const editor_asset_node_type_e node_type  = asset_node.type;

		_database.update_node_path(node, new_path);

		if (_source_file_track_inited && (node_type == editor_asset_node_type_e::folder || node_type == editor_asset_node_type_e::file))
			update_moved_source_paths(old_path.c_str(), new_path, node_type == editor_asset_node_type_e::folder);

		bool scripts_changed = false;

		if (_script_file_track_inited && (node_type == editor_asset_node_type_e::folder || node_type == editor_asset_node_type_e::script_file))
			scripts_changed = update_moved_script_paths(old_path.c_str(), new_path, node_type == editor_asset_node_type_e::folder);

		if (scripts_changed)
			_script_compile_requested = true;

		notify_changed();
	}

	void editor_asset_manager_t::move_node(editor_asset_node_handle_t node, editor_asset_node_handle_t new_parent, const char* new_path)
	{
		const editor_asset_node_t&	   asset_node = _database.get_asset_tree().value(node);
		const string_t				   old_path	  = asset_node.full_path;
		const editor_asset_node_type_e node_type  = asset_node.type;

		_database.move_node(node, new_parent, new_path);

		if (_source_file_track_inited && (node_type == editor_asset_node_type_e::folder || node_type == editor_asset_node_type_e::file))
			update_moved_source_paths(old_path.c_str(), new_path, node_type == editor_asset_node_type_e::folder);

		bool scripts_changed = false;

		if (_script_file_track_inited && (node_type == editor_asset_node_type_e::folder || node_type == editor_asset_node_type_e::script_file))
			scripts_changed = update_moved_script_paths(old_path.c_str(), new_path, node_type == editor_asset_node_type_e::folder);

		if (scripts_changed)
			_script_compile_requested = true;

		notify_changed();
	}

	void editor_asset_manager_t::notify_changed()
	{
		_generation++;
	}

	void editor_asset_manager_t::process_file_changes(span_t<const editor_file_change_t> changes)
	{
		bool scripts_changed = false;

		for (size_t i = 0; i < changes.size; ++i)
		{
			const editor_file_change_t& change = changes.data[i];

			if (change.type == editor_file_change_type_e::overflow)
			{
				if (change.root == editor_file_watch_root_e::cache)
					SFG_WARN("cache file notifications overflowed, reconciling tracked files");
				else
					SFG_WARN("asset file notifications overflowed, reconciling tracked files");

				_file_reconcile_root_mask |= static_cast<u8>(1u << static_cast<u8>(change.root));
				continue;
			}

			if (change.root == editor_file_watch_root_e::cache)
			{
				const auto cooked_it = _cooked_path_to_resource.find(change.path_id);

				if (cooked_it != _cooked_path_to_resource.end())
					process_cooked_resource_change(cooked_it->second);

				continue;
			}

			process_source_file_change(change.path_id);
			scripts_changed = process_script_file_change(change.path_id) || scripts_changed;
		}

		if (scripts_changed)
			_script_compile_requested = true;
	}

	void editor_asset_manager_t::process_file_reconciliation()
	{
		if (_file_reconcile_entries.empty() && _file_reconcile_root_mask != 0)
		{
			const u8 root_mask		  = _file_reconcile_root_mask;
			_file_reconcile_root_mask = 0;
			_file_reconcile_index	  = 0;

			if ((root_mask & (1u << static_cast<u8>(editor_file_watch_root_e::cache))) != 0)
			{
				for (const auto& tracking_pair : _cooked_resource_tracking_states)
				{
					_file_reconcile_entries.push_back({
						.id	  = tracking_pair.first,
						.kind = file_reconcile_kind_e::cooked,
					});
				}
			}

			if ((root_mask & (1u << static_cast<u8>(editor_file_watch_root_e::assets))) != 0)
			{
				for (const auto& tracking_pair : _source_file_to_tracking)
				{
					_file_reconcile_entries.push_back({
						.id	  = tracking_pair.first,
						.kind = file_reconcile_kind_e::source,
					});
				}

				for (const auto& tracking_pair : _script_file_tracking_states)
				{
					_file_reconcile_entries.push_back({
						.id	  = tracking_pair.first,
						.kind = file_reconcile_kind_e::script,
					});
				}
			}
		}

		bool scripts_changed = false;
		u32	 processed_count = 0;

		while (_file_reconcile_index < _file_reconcile_entries.size() && processed_count < EDITOR_FILE_RECONCILE_MAX_PER_TICK)
		{
			const file_reconcile_entry_t& entry = _file_reconcile_entries[_file_reconcile_index];

			switch (entry.kind)
			{
			case file_reconcile_kind_e::cooked:
				process_cooked_resource_change(entry.id);
				break;
			case file_reconcile_kind_e::source:
				process_source_file_change(entry.id);
				break;
			case file_reconcile_kind_e::script:
				scripts_changed = process_script_file_change(entry.id) || scripts_changed;
				break;
			}

			++_file_reconcile_index;
			++processed_count;
		}

		if (scripts_changed)
			_script_compile_requested = true;

		if (_file_reconcile_index == _file_reconcile_entries.size())
		{
			_file_reconcile_entries.resize(0);
			_file_reconcile_index = 0;
		}
	}

	bool editor_asset_manager_t::save_and_cook_embedded_asset_async(sid_t asset_id, const nlohmann::json& embedded_source)
	{
		editor_asset_t* asset = _database.find_asset(asset_id);
		if (asset == nullptr)
		{
			SFG_ERR("failed to find embedded asset {0}", asset_id);
			return false;
		}

		if (asset->source_type != editor_asset_source_type_e::embedded)
		{
			SFG_ERR("asset {0} does not have an embedded source", asset_id);
			return false;
		}

		const editor_asset_node_handle_t node = _database.find_asset_node(asset_id);

		if (node.is_null())
		{
			SFG_ERR("failed to find embedded asset node {0}", asset_id);
			return false;
		}

		const editor_asset_node_t& asset_node = _database.get_asset_tree().value(node);

		editor_asset_io_t::set_embedded_source_json(*asset, embedded_source);
		notify_changed();

		schedule_asset_cook(asset_id, *asset, asset_node.full_path.c_str(), asset_node.name.c_str(), true);

		return true;
	}

	bool editor_asset_manager_t::save_and_cook_file_asset_options_async(sid_t asset_id, const nlohmann::json& cook_options)
	{
		editor_asset_t* asset = _database.find_asset(asset_id);

		if (asset == nullptr)
		{
			SFG_ERR("failed to find file asset {0}", asset_id);
			return false;
		}

		if (asset->source_type != editor_asset_source_type_e::file && asset->source_type != editor_asset_source_type_e::file_blob)
		{
			SFG_ERR("asset {0} does not have a file source", asset_id);
			return false;
		}

		const editor_asset_node_handle_t node = _database.find_asset_node(asset_id);

		if (node.is_null())
		{
			SFG_ERR("failed to find file asset node {0}", asset_id);
			return false;
		}

		const editor_asset_node_t& asset_node = _database.get_asset_tree().value(node);

		editor_asset_io_t::set_cook_options_json(*asset, cook_options);
		notify_changed();

		schedule_asset_cook(asset_id, *asset, asset_node.full_path.c_str(), asset_node.name.c_str(), true);

		return true;
	}

	bool editor_asset_manager_t::cook_asset_async(sid_t asset_id)
	{
		const editor_asset_t* asset = _database.find_asset(asset_id);
		if (asset == nullptr)
		{
			SFG_ERR("failed to find file source asset {0}", asset_id);
			return false;
		}

		if (asset->source_type != editor_asset_source_type_e::file && asset->source_type != editor_asset_source_type_e::file_blob)
		{
			SFG_ERR("asset {0} does not have a file source", asset_id);
			return false;
		}

		const editor_asset_node_handle_t node = _database.find_asset_node(asset_id);
		if (node.is_null())
		{
			SFG_ERR("failed to find file source asset node {0}", asset_id);
			return false;
		}

		const editor_asset_node_t& asset_node = _database.get_asset_tree().value(node);

		schedule_asset_cook(asset_id, *asset, asset_node.full_path.c_str(), asset_node.name.c_str(), false);

		return true;
	}

	void editor_asset_manager_t::schedule_asset_cook(sid_t asset_id, editor_asset_t asset, const char* asset_path, const char* display_name, bool save_asset)
	{
		const auto cook_handle = _asset_cook_states.emplace();

		SFG_ASSERT(!cook_handle.is_null());

		asset_cook_state_t& cook_state = _asset_cook_states.get(cook_handle);
		cook_state.asset			   = std::move(asset);
		cook_state.asset_path		   = asset_path;
		cook_state.display_name		   = display_name;
		cook_state.handle			   = cook_handle;
		cook_state.revision			   = _next_asset_cook_revision++;
		cook_state.save_asset		   = save_asset;

		if (cook_state.asset.asset_type == editor_asset_type_e::shader)
			_latest_asset_cook_revisions[asset_id] = cook_state.revision;

		_last_asset_cook_work = editor_app_t::get().get_work_controller().submit_work({
			.fn =
				[](editor_work_context_t& context, void* user_data) {
					asset_cook_state_t& state = *static_cast<asset_cook_state_t*>(user_data);

					return editor_asset_manager_t::get().asset_cook_worker(state);
				},
			.completed =
				[](editor_work_handle_t handle, editor_work_state_e state, void* user_data) {
					asset_cook_state_t& cook_state = *static_cast<asset_cook_state_t*>(user_data);

					editor_asset_manager_t::get().complete_asset_cook(handle, state == editor_work_state_e::succeeded, cook_state);
				},
			.user_data		= &cook_state,
			.initial_status = "Cooking asset",
		});
		_asset_cook_work_count++;
	}

	void editor_asset_manager_t::flush_asset_cook_jobs()
	{
		while (_asset_cook_work_count != 0)
		{
			SFG_ASSERT(!_last_asset_cook_work.is_null());

			const editor_work_handle_t work = _last_asset_cook_work;

			editor_app_t::get().get_work_controller().wait_for_work(work);
		}

		SFG_ASSERT(_last_asset_cook_work.is_null());
		SFG_ASSERT(_asset_cook_states.begin() == _asset_cook_states.end());
	}

	bool editor_asset_manager_t::asset_cook_worker(asset_cook_state_t& cook_state)
	{
		const sid_t asset_id	 = cook_state.asset.guid;
		const bool	asset_saved	 = !cook_state.save_asset || editor_asset_io_t::write_asset(cook_state.asset_path.c_str(), cook_state.asset);
		bool		asset_cooked = false;

		if (asset_saved && cook_state.asset.asset_type == editor_asset_type_e::shader)
		{
			shader_data_definition_t definition = {};
			asset_cooked						= editor_asset_cooker_t::cook_shader(cook_state.asset, cook_state.display_name.c_str(), &definition);

			if (asset_cooked)
				editor_asset_io_t::set_embedded_source_json(cook_state.asset, definition);
		}
		else if (asset_saved)
			asset_cooked = editor_asset_cooker_t::cook_asset(cook_state.asset, cook_state.display_name.c_str());

		if (!asset_cooked)
		{
			if (cook_state.save_asset)
				SFG_ERR("failed to save and cook embedded asset {0}", asset_id);
			else
				SFG_ERR("failed to cook file source asset {0}", asset_id);
		}
		else if (cook_state.save_asset)
			SFG_TRACE("asynchronously saved and cooked asset! {0}", cook_state.display_name);
		else
			SFG_TRACE("asynchronously cooked asset! {0}", cook_state.display_name);

		return asset_cooked;
	}

	void editor_asset_manager_t::complete_asset_cook(editor_work_handle_t work_handle, bool succeeded, asset_cook_state_t& cook_state)
	{
		SFG_ASSERT(_asset_cook_work_count != 0);

		_asset_cook_work_count--;

		if (work_handle == _last_asset_cook_work)
			_last_asset_cook_work = {};

		bool shader_definitions_changed = false;

		if (cook_state.asset.asset_type == editor_asset_type_e::shader)
		{
			const auto latest = _latest_asset_cook_revisions.find(cook_state.asset.guid);

			SFG_ASSERT(latest != _latest_asset_cook_revisions.end());

			if (latest->second == cook_state.revision)
			{
				_latest_asset_cook_revisions.erase(latest);

				if (succeeded)
					shader_definitions_changed = process_completed_shader_cook(cook_state.asset);
			}
		}

		if (shader_definitions_changed)
		{
			if (editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::inspector))
				static_cast<editor_panel_inspector_t*>(panel)->refresh_from_assets();
		}

		const auto cook_handle = cook_state.handle;

		_asset_cook_states.remove(cook_handle);
	}

	bool editor_asset_manager_t::process_completed_shader_cook(const editor_asset_t& cooked_shader)
	{
		editor_asset_t* shader_asset = _database.find_asset(cooked_shader.guid);
		if (shader_asset == nullptr || shader_asset->asset_type != editor_asset_type_e::shader)
			return false;

		const editor_asset_node_handle_t shader_node = _database.find_asset_node(cooked_shader.guid);

		if (shader_node.is_null())
			return false;

		const nlohmann::json	 definition_json = editor_asset_io_t::get_embedded_source_json(cooked_shader);
		shader_data_definition_t definition		 = {};
		definition_json.get_to(definition);

		if (definition_json == editor_asset_io_t::get_embedded_source_json(*shader_asset))
			return false;

		editor_asset_t updated_shader = *shader_asset;
		editor_asset_io_t::set_embedded_source_json(updated_shader, definition_json);

		const editor_asset_node_t& shader_node_value = _database.get_asset_tree().value(shader_node);

		if (!editor_asset_io_t::write_asset(shader_node_value.full_path.c_str(), updated_shader))
			SFG_ERR("failed to persist reflected shader definition for asset {0}", cooked_shader.guid);

		*shader_asset = std::move(updated_shader);
		notify_changed();

		struct material_update_t
		{
			sid_t		   asset_id = NULL_SID;
			nlohmann::json embedded = {};
		};

		frame_vector_t<material_update_t> material_updates = {};

		for (const auto& asset_pair : _database.get_assets())
		{
			const editor_asset_t& material_asset = asset_pair.second;

			if (material_asset.asset_type != editor_asset_type_e::material)
				continue;

			const nlohmann::json embedded = editor_asset_io_t::get_embedded_source_json(material_asset);

			if (!embedded.is_object())
				continue;

			material_def_t material = {};
			embedded.get_to(material);

			if (material.shader != cooked_shader.guid)
				continue;

			const material_def_t refreshed		= refresh_material_from_shader_definition(material, definition);
			const nlohmann::json refreshed_json = refreshed;

			if (refreshed_json != embedded)
				material_updates.push_back({.asset_id = material_asset.guid, .embedded = refreshed_json});
		}

		for (const material_update_t& update : material_updates)
		{
			if (!save_and_cook_embedded_asset_async(update.asset_id, update.embedded))
				SFG_ERR("failed to synchronize material {0} with shader {1}", update.asset_id, cooked_shader.guid);
		}

		return true;
	}

	void editor_asset_manager_t::process_cooked_resource_change(sid_t resource_id)
	{
		const auto tracking_it = _cooked_resource_tracking_states.find(resource_id);

		if (tracking_it == _cooked_resource_tracking_states.end())
			return;

		cooked_resource_tracking_state_t& tracking_state = tracking_it->second;
		const u64						  last_modified	 = file_system_t::get_last_modified_ticks(tracking_state.cache_path.c_str());

		if (last_modified == 0 || tracking_state.last_modified == last_modified)
			return;

		tracking_state.last_modified = last_modified;

		SFG_TRACE("detected cooked resource change! {0}", tracking_state.cache_path);
		_changed_cooked_resources.push_back(resource_id);
	}

	void editor_asset_manager_t::process_changed_cooked_resources()
	{
		resource_manager_t&				  resource_manager	= resource_manager_t::get();
		editor_asset_thumbnail_manager_t& thumbnail_manager = editor_asset_thumbnail_manager_t::get();

		for (const sid_t resource_id : _changed_cooked_resources)
		{
			const auto tracking_it = _cooked_resource_tracking_states.find(resource_id);
			SFG_ASSERT(tracking_it != _cooked_resource_tracking_states.end());

			const cooked_resource_tracking_state_t& tracking_state = tracking_it->second;
			const editor_asset_t*					asset		   = _database.find_asset(tracking_state.asset_id);

			if (asset == nullptr)
				continue;

			if (tracking_state.kind == cooked_resource_kind_e::thumbnail)
				thumbnail_manager.refresh_thumbnail_resource(resource_id);
			else
			{
				if (resource_manager.find_entry(resource_id) != nullptr)
					resource_manager.reload_resource(resource_id);

				if (editor_asset_thumbnailer_t::is_renderable_thumbnail(asset->asset_type) && asset->thumbnail_guid != editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset->asset_type))
					thumbnail_manager.request_render(asset->guid);
			}
		}

		_changed_cooked_resources.resize(0);
	}

	void editor_asset_manager_t::track_cooked_resource(sid_t resource_id, sid_t asset_id, cooked_resource_kind_e kind, bool report_existing_file)
	{
		if (resource_id == NULL_SID)
			return;

		if (kind == cooked_resource_kind_e::thumbnail)
		{
			const editor_asset_t* asset = _database.find_asset(asset_id);

			SFG_ASSERT(asset != nullptr);

			if (resource_id == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset->asset_type))
				return;
		}

		auto [it, inserted] = _cooked_resource_tracking_states.try_emplace(resource_id);

		if (!inserted)
			return;

		cooked_resource_tracking_state_t& tracking_state = it->second;
		tracking_state.cache_path						 = editor_asset_path_t::get_cache_path_for_guid(resource_id);
		tracking_state.asset_id							 = asset_id;
		tracking_state.last_modified					 = file_system_t::get_last_modified_ticks(tracking_state.cache_path.c_str());
		tracking_state.kind								 = kind;

		const sid_t path_id					= editor_asset_path_t::hash_path(tracking_state.cache_path.c_str());
		const auto [path_it, path_inserted] = _cooked_path_to_resource.emplace(path_id, resource_id);

		SFG_ASSERT(path_inserted || path_it->second == resource_id);

		if (report_existing_file && tracking_state.last_modified != 0)
			_changed_cooked_resources.push_back(resource_id);
	}

	void editor_asset_manager_t::untrack_cooked_resource(sid_t resource_id)
	{
		const auto tracking_it = _cooked_resource_tracking_states.find(resource_id);

		if (tracking_it == _cooked_resource_tracking_states.end())
			return;

		_cooked_path_to_resource.erase(editor_asset_path_t::hash_path(tracking_it->second.cache_path.c_str()));
		_cooked_resource_tracking_states.erase(tracking_it);

		const auto changed_end = std::remove(_changed_cooked_resources.begin(), _changed_cooked_resources.end(), resource_id);
		_changed_cooked_resources.erase(changed_end, _changed_cooked_resources.end());
	}

	void editor_asset_manager_t::track_source_asset(const editor_asset_t& asset)
	{
		if ((asset.source_type != editor_asset_source_type_e::file && asset.source_type != editor_asset_source_type_e::file_blob) || asset.source_relative.empty() || !editor_asset_cooker_t::is_cookable(asset.asset_type))
			return;

		const string_t source_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		const sid_t	   source_id   = editor_asset_path_t::hash_path(source_path.c_str());
		const auto	   existing	   = _asset_to_source_tracking.find(asset.guid);

		if (existing != _asset_to_source_tracking.end())
		{
			if (existing->second == source_id)
			{
				const auto tracking_it = _source_file_to_tracking.find(source_id);
				SFG_ASSERT(tracking_it != _source_file_to_tracking.end());

				const source_file_tracking_state_t& tracking_state = tracking_it->second;
				SFG_ASSERT(editor_asset_path_t::is_same_path(tracking_state.full_path.c_str(), source_path.c_str()));
				return;
			}

			return;
		}

		auto [tracking_it, inserted]				 = _source_file_to_tracking.try_emplace(source_id);
		source_file_tracking_state_t& tracking_state = tracking_it->second;

		if (inserted)
		{
			tracking_state.full_path	 = source_path;
			tracking_state.last_modified = file_system_t::get_last_modified_ticks(source_path.c_str());
		}
		else
			SFG_ASSERT(editor_asset_path_t::is_same_path(tracking_state.full_path.c_str(), source_path.c_str()));

		tracking_state.asset_ids.push_back(asset.guid);
		_asset_to_source_tracking.emplace(asset.guid, source_id);
	}

	void editor_asset_manager_t::process_source_file_change(sid_t source_id)
	{
		const auto tracking_it = _source_file_to_tracking.find(source_id);

		if (tracking_it == _source_file_to_tracking.end())
			return;

		source_file_tracking_state_t& tracking_state = tracking_it->second;
		const u64					  last_modified	 = file_system_t::get_last_modified_ticks(tracking_state.full_path.c_str());

		if (last_modified == 0 || tracking_state.last_modified == last_modified)
			return;

		tracking_state.last_modified = last_modified;

		SFG_TRACE("detected source file change! {0}", tracking_state.full_path);

		for (const sid_t asset_id : tracking_state.asset_ids)
			_changed_source_assets.push_back(asset_id);
	}

	void editor_asset_manager_t::process_changed_source_files()
	{
		for (const sid_t asset_id : _changed_source_assets)
			cook_asset_async(asset_id);

		_changed_source_assets.resize(0);
	}

	void editor_asset_manager_t::untrack_source_asset(sid_t asset_id)
	{
		const auto source_asset_it = _asset_to_source_tracking.find(asset_id);

		if (source_asset_it == _asset_to_source_tracking.end())
			return;

		const auto tracking_it = _source_file_to_tracking.find(source_asset_it->second);
		SFG_ASSERT(tracking_it != _source_file_to_tracking.end());

		vector_t<sid_t>& asset_ids = tracking_it->second.asset_ids;
		const auto		 asset_it  = std::find(asset_ids.begin(), asset_ids.end(), asset_id);
		SFG_ASSERT(asset_it != asset_ids.end());

		asset_ids.erase(asset_it);

		if (asset_ids.empty())
			_source_file_to_tracking.erase(tracking_it);

		_asset_to_source_tracking.erase(source_asset_it);
		const auto changed_end = std::remove(_changed_source_assets.begin(), _changed_source_assets.end(), asset_id);
		_changed_source_assets.erase(changed_end, _changed_source_assets.end());
	}

	void editor_asset_manager_t::update_moved_source_paths(const char* old_path, const char* new_path, bool directory)
	{
		const string_t	old_absolute_path = file_system_t::get_absolute_path(old_path);
		const string_t	new_absolute_path = file_system_t::get_absolute_path(new_path);
		const string_t	old_directory	  = directory ? editor_asset_path_t::normalize_directory(old_absolute_path.c_str()) : "";
		const string_t	new_directory	  = directory ? editor_asset_path_t::normalize_directory(new_absolute_path.c_str()) : "";
		const string_t& assets_path		  = editor_project_t::get()._runtime.assets_path;

		for (auto& asset_pair : _database.get_assets())
		{
			editor_asset_t& asset = asset_pair.second;

			if ((asset.source_type != editor_asset_source_type_e::file && asset.source_type != editor_asset_source_type_e::file_blob) || asset.source_relative.empty())
				continue;

			const string_t source_path	= editor_asset_path_t::get_source_full_path(assets_path.c_str(), asset);
			const bool	   source_moved = directory ? editor_asset_path_t::is_path_in_directory(source_path.c_str(), old_directory.c_str()) : editor_asset_path_t::is_same_path(source_path.c_str(), old_absolute_path.c_str());

			if (!source_moved)
				continue;

			string_t moved_source_path = new_absolute_path;

			if (directory)
			{
				moved_source_path = new_directory;
				moved_source_path += source_path.substr(old_directory.size());
			}

			asset.source_relative = editor_asset_path_t::get_source_relative(assets_path.c_str(), moved_source_path.c_str());
			track_source_asset(asset);
		}
	}

	void editor_asset_manager_t::track_script_file(const char* path)
	{
		const string_t absolute_path = file_system_t::get_absolute_path(path);
		const sid_t	   path_id		 = editor_asset_path_t::hash_path(absolute_path.c_str());
		auto [tracking_it, inserted] = _script_file_tracking_states.try_emplace(path_id);

		SFG_ASSERT(inserted);

		script_file_tracking_state_t& tracking_state = tracking_it->second;
		tracking_state.full_path					 = absolute_path;
		tracking_state.last_modified				 = file_system_t::get_last_modified_ticks(absolute_path.c_str());
	}

	bool editor_asset_manager_t::process_script_file_change(sid_t script_id)
	{
		const auto tracking_it = _script_file_tracking_states.find(script_id);

		if (tracking_it == _script_file_tracking_states.end())
			return false;

		script_file_tracking_state_t& tracking_state = tracking_it->second;
		const u64					  last_modified	 = file_system_t::get_last_modified_ticks(tracking_state.full_path.c_str());

		if (tracking_state.last_modified == last_modified)
			return false;

		tracking_state.last_modified = last_modified;

		SFG_TRACE("detected C# script file change: {0}", tracking_state.full_path);
		return true;
	}

	bool editor_asset_manager_t::update_moved_script_paths(const char* old_path, const char* new_path, bool directory)
	{
		struct moved_script_file_t
		{
			string_t path	= {};
			sid_t	 old_id = NULL_SID;
		};

		const string_t				  old_absolute_path = file_system_t::get_absolute_path(old_path);
		const string_t				  new_absolute_path = file_system_t::get_absolute_path(new_path);
		const string_t				  old_directory		= directory ? editor_asset_path_t::normalize_directory(old_absolute_path.c_str()) : "";
		const string_t				  new_directory		= directory ? editor_asset_path_t::normalize_directory(new_absolute_path.c_str()) : "";
		vector_t<moved_script_file_t> moved_files		= {};

		for (const auto& tracking_pair : _script_file_tracking_states)
		{
			const script_file_tracking_state_t& tracking_state = tracking_pair.second;
			const bool script_moved = directory ? editor_asset_path_t::is_path_in_directory(tracking_state.full_path.c_str(), old_directory.c_str()) : editor_asset_path_t::is_same_path(tracking_state.full_path.c_str(), old_absolute_path.c_str());

			if (!script_moved)
				continue;

			string_t moved_path = new_absolute_path;

			if (directory)
			{
				moved_path = new_directory;
				moved_path += tracking_state.full_path.substr(old_directory.size());
			}

			moved_files.push_back({
				.path	= std::move(moved_path),
				.old_id = tracking_pair.first,
			});
		}

		for (const moved_script_file_t& moved_file : moved_files)
		{
			_script_file_tracking_states.erase(moved_file.old_id);
			track_script_file(moved_file.path.c_str());
		}

		return !moved_files.empty();
	}

	const editor_asset_descriptor_t* editor_asset_manager_t::find_asset_descriptor(const string_t& extension) const
	{
		const auto descriptor_it = std::find_if(_asset_descriptors.begin(), _asset_descriptors.end(), [&](const auto& asset_descriptor) {
			const editor_asset_descriptor_t& descriptor	  = asset_descriptor.second;
			const auto						 extension_it = std::find_if(descriptor.extensions.begin(), descriptor.extensions.end(), [&](const string_t& descriptor_extension) { return descriptor_extension == extension; });
			return extension_it != descriptor.extensions.end();
		});
		return descriptor_it != _asset_descriptors.end() ? &descriptor_it->second : nullptr;
	}

	void editor_asset_manager_t::import_assets(editor_asset_node_handle_t directory_node, const frame_vector_t<string_t>& paths, const frame_vector_t<editor_asset_import_options_t>& import_options)
	{
		SFG_ASSERT(!_import_in_progress);

		const editor_asset_tree_t& tree		 = _database.get_asset_tree();
		const editor_asset_node_t& directory = tree.value(directory_node);

		const auto status_path_it = std::find_if(paths.begin(), paths.end(), [](const string_t& path) { return !path.empty(); });

		SFG_ASSERT(status_path_it != paths.end());

		_import_state.target_directory = directory.full_path;
		_import_state.paths.resize(0);
		_import_state.paths.reserve(paths.size());
		_import_state.import_options.resize(0);
		_import_state.import_options.reserve(import_options.size());
		_import_state.imported_asset_paths.resize(0);

		for (const string_t& path : paths)
			_import_state.paths.push_back(path);

		for (const editor_asset_import_options_t& options : import_options)
			_import_state.import_options.push_back(options);

		_import_work_status_text.resize(0);
		_import_work_progress		  = 0.0f;
		_import_displayed_status	  = *status_path_it;
		_import_in_progress			  = true;
		_import_target_directory_node = directory_node;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_import_progress_modal.set_progress(0.0f);
		const editor_modal_content_desc_t progress_content = _import_progress_modal.get_content_desc();

		modal.request_modal("Importing Assets", _import_displayed_status.c_str(), false, nullptr, 0, &progress_content);

		_import_work = editor_app_t::get().get_work_controller().submit_work({
			.fn =
				[](editor_work_context_t& context, void* user_data) {
					editor_asset_manager_t& asset_manager = *static_cast<editor_asset_manager_t*>(user_data);
					asset_import_state_t&	import_state  = asset_manager._import_state;

					editor_asset_manager_util_t::import_assets(import_state.target_directory.c_str(),
															   {.data = import_state.paths.data(), .size = import_state.paths.size()},
															   {.data = import_state.import_options.data(), .size = import_state.import_options.size()},
															   context,
															   import_state.imported_asset_paths);

					return true;
				},
			.completed =
				[](editor_work_handle_t handle, editor_work_state_e state, void* user_data) {
					editor_asset_manager_t& asset_manager = *static_cast<editor_asset_manager_t*>(user_data);

					SFG_ASSERT(handle == asset_manager._import_work);

					asset_manager._import_work = {};

					if (!editor_surface_controller_t::get().is_empty())
					{
						editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

						modal.close_modal();
						asset_manager.sync_imported_asset_paths(asset_manager._import_target_directory_node, {.data = asset_manager._import_state.imported_asset_paths.data(), .size = asset_manager._import_state.imported_asset_paths.size()});
					}

					asset_manager._import_state.target_directory.resize(0);
					asset_manager._import_state.paths.resize(0);
					asset_manager._import_state.imported_asset_paths.resize(0);
					asset_manager._import_state.import_options.resize(0);
					asset_manager._import_work_status_text.resize(0);
					asset_manager._import_work_progress = 0.0f;
					asset_manager._import_displayed_status.resize(0);
					asset_manager._import_target_directory_node = {};
					asset_manager._import_in_progress			= false;
				},
			.user_data		= this,
			.initial_status = _import_displayed_status.c_str(),
		});
	}

	editor_asset_node_handle_t editor_asset_manager_t::find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const
	{
		const editor_asset_tree_t& tree	 = _database.get_asset_tree();
		editor_asset_node_handle_t child = tree.first_child(parent);
		while (!child.is_null())
		{
			const editor_asset_node_t& node = tree.value(child);
			if (node.type == editor_asset_node_type_e::folder && node.name == name)
				return child;

			child = tree.next_sibling(child);
		}
		return {};
	}

	editor_asset_node_handle_t editor_asset_manager_t::get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name)
	{
		const editor_asset_node_handle_t existing = find_child_folder(parent, name);
		if (!existing.is_null())
			return existing;

		const u8				   flags	 = !name.empty() && name[0] == '_' ? editor_asset_node_flag_hidden : 0;
		const editor_asset_tree_t& tree		 = _database.get_asset_tree();
		string_t				   full_path = editor_asset_path_t::normalize_directory(tree.value(parent).full_path.c_str());
		full_path += name;
		const editor_asset_node_handle_t folder = _database.emplace_node(editor_asset_node_t{.name = name, .full_path = full_path, .type = editor_asset_node_type_e::folder, .flags = flags});
		_database.attach_node(parent, folder);
		return folder;
	}
}
