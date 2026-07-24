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
#include "editor_surface_controller.hpp"
#include "editor_project.hpp"
#include "ui/editor_modal_controller.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/frame_hash_map.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/color_utils.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>
#include <utility>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define EDITOR_ASSET_COLOR(R, G, B)			 color_utils_t::srgb_to_linear(color_t::from255(R, G, B, 255.0f)).to_vector()
#define EDITOR_ASSET_DELETION_LISTENER_MAX	 64
#define EDITOR_COOKED_RESOURCE_SCAN_INTERVAL 30

	bool editor_asset_manager_t::init()
	{
		SFG_ASSERT(s_instance == nullptr);

		s_instance = this;

		_asset_deletion_listeners.init(EDITOR_ASSET_DELETION_LISTENER_MAX);
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
		register_descriptor({.extensions = {"hdr"}, .display_name = "HDR Skybox", .color = EDITOR_ASSET_COLOR(87.0f, 175.0f, 142.0f), .asset_type = editor_asset_type_e::hdr_skybox});
		register_descriptor({.display_name = "Physics Collision Mesh", .color = EDITOR_ASSET_COLOR(214.0f, 96.0f, 57.0f), .asset_type = editor_asset_type_e::physics_collision_mesh});
		register_descriptor({.display_name = "World", .color = EDITOR_ASSET_COLOR(98.0f, 212.0f, 205.0f), .asset_type = editor_asset_type_e::world});

		clear();

		return true;
	}

	void editor_asset_manager_t::uninit()
	{
		SFG_ASSERT(s_instance == this);

		flush_asset_save_cook_jobs();

		if (_import_in_progress)
			editor_app_t::get().get_editor_work_executor().wait_for_all();

		clear();

		_import_status_pending.clear();
		_import_status_visible.clear();
		_import_asset_paths_pending.clear();
		_import_asset_paths_visible.clear();
		_asset_descriptors.clear();
		_asset_deletion_listeners.uninit();

		{
			LOCK_GUARD(_asset_save_cook_mtx);
			_asset_save_cook_states.clear();
		}

		_import_status_dirty.store(false, std::memory_order_relaxed);
		_asset_save_cook_worker_count.store(0, std::memory_order_relaxed);
		_next_asset_save_cook_revision = 1;

		_import_progress_pending				 = 0.0f;
		_cooked_resource_scan_ticks				 = 0;
		_import_completed_pending				 = false;
		_import_in_progress						 = false;
		_is_cooked_resource_tracking_initialized = false;
		_import_target_directory_node			 = {};

		s_instance = nullptr;
	}

	void editor_asset_manager_t::tick()
	{
		ZoneScoped;

		// import modal
		if (_import_in_progress && _import_status_dirty.exchange(false, std::memory_order_acquire))
		{
			f32		 progress	  = 0.0f;
			bool	 is_completed = false;
			string_t status		  = {};

			{
				LOCK_GUARD(_import_status_mtx);
				progress					= _import_progress_pending;
				is_completed				= _import_completed_pending;
				_import_status_visible		= _import_status_pending;
				_import_asset_paths_visible = _import_asset_paths_pending;
				status						= _import_status_visible;
			}

			editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;
			_import_progress_modal.set_progress(progress);

			if (!status.empty())
				modal.set_body_text(status.c_str());

			if (is_completed)
			{
				modal.close_modal();
				sync_imported_asset_paths(_import_target_directory_node, {.data = _import_asset_paths_visible.data(), .size = _import_asset_paths_visible.size()});
				_import_status_pending.resize(0);
				_import_status_visible.resize(0);
				_import_asset_paths_pending.resize(0);
				_import_asset_paths_visible.resize(0);
				_import_progress_pending	  = 0.0f;
				_import_completed_pending	  = false;
				_import_in_progress			  = false;
				_import_target_directory_node = {};
			}
		}

		// integrity check.
		if (_last_integrity_generation != _generation && !_database.get_root_node().is_null())
		{
			editor_asset_manager_util_t::ensure_integrity(*this);
			_last_integrity_generation = _generation;
		}

		// track file changes for cooked resources
		if (_is_cooked_resource_tracking_initialized)
		{
			++_cooked_resource_scan_ticks;

			if (_cooked_resource_scan_ticks >= EDITOR_COOKED_RESOURCE_SCAN_INTERVAL)
			{
				_cooked_resource_scan_ticks = 0;
				scan_cooked_resources();
				process_changed_cooked_resources();
			}
		}
	}

	void editor_asset_manager_t::clear()
	{
		SFG_ASSERT(_asset_save_cook_worker_count.load(std::memory_order_acquire) == 0);

		_database.clear();
		_cooked_resource_tracking_states.clear();
		_changed_cooked_resources.resize(0);
		_cooked_resource_scan_ticks				 = 0;
		_is_cooked_resource_tracking_initialized = false;
		_generation++;
	}

	void editor_asset_manager_t::initialize_cooked_resource_tracking()
	{
		_cooked_resource_tracking_states.clear();
		_cooked_resource_tracking_states.reserve(_database.get_assets().size() * 2);
		_changed_cooked_resources.resize(0);
		_cooked_resource_scan_ticks = 0;

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

		_is_cooked_resource_tracking_initialized = true;
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

			if (_is_cooked_resource_tracking_initialized)
			{
				track_cooked_resource(asset_id, asset_id, cooked_resource_kind_e::asset, true);
				track_cooked_resource(thumbnail_id, asset_id, cooked_resource_kind_e::thumbnail, true);
			}

			notify_changed();
			return node;
		}

		const string_t					 name = file_system_t::get_filename_and_extension_from_path(path);
		const editor_asset_node_handle_t node = _database.emplace_node(editor_asset_node_t{.name = name, .full_path = path, .type = editor_asset_node_type_e::file});
		_database.attach_node(parent, node);
		notify_changed();
		return node;
	}

	editor_asset_node_handle_t editor_asset_manager_t::add_directory_tree(editor_asset_node_handle_t parent, const char* path)
	{
		const editor_asset_node_handle_t directory = add_folder_node(parent, path);
		vector_t<file_system_entry_t>	 entries;
		file_system_t::get_entries_recursive(path, entries);

		vector_t<string_t> parts;
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

		_database.erase_asset(old_guid);
		asset_node.asset_id = asset_id;
		_database.upsert_asset(std::move(asset));
		_database.rebuild_indices();

		if (_is_cooked_resource_tracking_initialized)
		{
			track_cooked_resource(asset_id, asset_id, cooked_resource_kind_e::asset, true);
			track_cooked_resource(thumbnail_id, asset_id, cooked_resource_kind_e::thumbnail, true);
		}

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

		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(directory_path.c_str(), entries);

		vector_t<string_t> parts;
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

		vector_t<string_t> parts;
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

		frame_hash_map_t<sid_t, bool> deleted_asset_ids = {};
		deleted_asset_ids.reserve(deleted_asset_count);

		frame_hash_map_t<u64, string_t> deleted_blob_paths = {};
		deleted_blob_paths.reserve(deleted_asset_count);

		tree.for_each_depth_first(node, [&](editor_asset_node_handle_t current, u32 depth) {
			const editor_asset_node_t& current_node = tree.value(current);

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
		flush_asset_save_cook_jobs();

		// delete the main path first
		const bool removed_from_disk = root_node.type == editor_asset_node_type_e::folder ? file_system_t::delete_directory(root_path.c_str()) : !file_system_t::exists(root_path.c_str()) || !file_system_t::delete_file(root_path.c_str());

		if (!removed_from_disk)
		{
			SFG_ERR("failed to delete {0}", root_path);
			return false;
		}

		// let consumers release deleted assets before force-unload
		notify_asset_deletion({.data = deleted_asset_guids.data(), .size = deleted_asset_guids.size()});

		// cancel thumbnail work and unload live resources
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
		}

		// commit deletion to the asset database
		_database.remove_node_subtree(node);
		notify_changed();

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
		_database.update_node_path(node, new_path);
		notify_changed();
	}

	void editor_asset_manager_t::move_node(editor_asset_node_handle_t node, editor_asset_node_handle_t new_parent, const char* new_path)
	{
		_database.move_node(node, new_parent, new_path);
		notify_changed();
	}

	void editor_asset_manager_t::notify_changed()
	{
		_generation++;
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
		editor_asset_t			   updated	  = *asset;

		editor_asset_io_t::set_embedded_source_json(updated, embedded_source);
		*asset = updated;
		notify_changed();

		bool schedule_worker = false;

		{
			LOCK_GUARD(_asset_save_cook_mtx);

			auto [it, inserted]						 = _asset_save_cook_states.try_emplace(asset_id);
			asset_save_cook_state_t& save_cook_state = it->second;
			save_cook_state.asset					 = std::move(updated);
			save_cook_state.asset_path				 = asset_node.full_path;
			save_cook_state.display_name			 = asset_node.name;
			save_cook_state.revision				 = _next_asset_save_cook_revision++;
			schedule_worker							 = inserted;

			if (schedule_worker)
				_asset_save_cook_worker_count.fetch_add(1, std::memory_order_release);
		}

		if (schedule_worker)
			editor_app_t::get().get_editor_work_executor().silent_async([this, asset_id]() { save_and_cook_asset_worker(asset_id); });

		return true;
	}

	void editor_asset_manager_t::flush_asset_save_cook_jobs()
	{
		if (_asset_save_cook_worker_count.load(std::memory_order_acquire) == 0)
			return;

		editor_app_t::get().get_editor_work_executor().wait_for_all();
		SFG_ASSERT(_asset_save_cook_worker_count.load(std::memory_order_acquire) == 0);
	}

	void editor_asset_manager_t::save_and_cook_asset_worker(sid_t asset_id)
	{
		while (true)
		{
			asset_save_cook_state_t save_cook_state = {};

			{
				LOCK_GUARD(_asset_save_cook_mtx);

				const auto it = _asset_save_cook_states.find(asset_id);
				SFG_ASSERT(it != _asset_save_cook_states.end());
				save_cook_state = it->second;
			}

			const bool saved_and_cooked = editor_asset_io_t::write_asset(save_cook_state.asset_path.c_str(), save_cook_state.asset) && editor_asset_cooker_t::cook_asset(save_cook_state.asset, save_cook_state.display_name.c_str());

			if (!saved_and_cooked)
				SFG_ERR("failed to save and cook embedded asset {0}", asset_id);
			else
				SFG_TRACE("asyncly saved and cooked asset! {0}", save_cook_state.display_name);

			bool is_completed = false;

			{
				LOCK_GUARD(_asset_save_cook_mtx);

				const auto it = _asset_save_cook_states.find(asset_id);
				SFG_ASSERT(it != _asset_save_cook_states.end());

				if (it->second.revision == save_cook_state.revision)
				{
					_asset_save_cook_states.erase(it);
					_asset_save_cook_worker_count.fetch_sub(1, std::memory_order_release);
					is_completed = true;
				}
			}

			if (is_completed)
				return;
		}
	}

	void editor_asset_manager_t::scan_cooked_resources()
	{
		for (auto& tracking_pair : _cooked_resource_tracking_states)
		{
			const sid_t						  resource_id	 = tracking_pair.first;
			cooked_resource_tracking_state_t& tracking_state = tracking_pair.second;
			const u64						  last_modified	 = file_system_t::get_last_modified_ticks(tracking_state.cache_path.c_str());

			if (last_modified == 0 || tracking_state.last_modified == last_modified)
				continue;

			tracking_state.last_modified = last_modified;

			SFG_TRACE("detected cooked resource change! {0}", tracking_state.cache_path);
			_changed_cooked_resources.push_back(resource_id);
		}
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
		tracking_state.last_modified					 = report_existing_file ? 0 : file_system_t::get_last_modified_ticks(tracking_state.cache_path.c_str());
		tracking_state.kind								 = kind;
	}

	void editor_asset_manager_t::untrack_cooked_resource(sid_t resource_id)
	{
		_cooked_resource_tracking_states.erase(resource_id);

		const auto changed_end = std::remove(_changed_cooked_resources.begin(), _changed_cooked_resources.end(), resource_id);
		_changed_cooked_resources.erase(changed_end, _changed_cooked_resources.end());
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
		const editor_asset_tree_t& tree		 = _database.get_asset_tree();
		const editor_asset_node_t& directory = tree.value(directory_node);

		const auto status_path_it = std::find_if(paths.begin(), paths.end(), [](const string_t& path) { return !path.empty(); });
		SFG_ASSERT(status_path_it != paths.end());
		_import_status_pending = *status_path_it;
		_import_status_visible = _import_status_pending;
		_import_asset_paths_pending.resize(0);
		_import_asset_paths_visible.resize(0);
		_import_status_dirty.store(false, std::memory_order_relaxed);
		_import_progress_pending	  = 0.0f;
		_import_completed_pending	  = false;
		_import_in_progress			  = true;
		_import_target_directory_node = directory_node;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		_import_progress_modal.set_progress(0.0f);
		editor_modal_content_desc_t progress_content = _import_progress_modal.get_content_desc();
		modal.request_modal("Importing Assets", _import_status_visible.c_str(), false, nullptr, 0, &progress_content);

		const span_t<const string_t> import_paths = {
			.data = paths.data(),
			.size = paths.size(),
		};
		const span_t<const editor_asset_import_options_t> options = {
			.data = import_options.data(),
			.size = import_options.size(),
		};

		editor_asset_manager_util_t::import_assets_async(directory.full_path.c_str(), import_paths, options, editor_app_t::get().get_editor_work_executor(), on_import_progress, this);
	}

	void editor_asset_manager_t::on_import_progress(void* user_data, f32 progress, const char* text, bool is_completed, span_t<const string_t> imported_asset_paths)
	{
		editor_asset_manager_t& asset_manager = *static_cast<editor_asset_manager_t*>(user_data);
		{
			LOCK_GUARD(asset_manager._import_status_mtx);
			asset_manager._import_progress_pending	= progress;
			asset_manager._import_completed_pending = is_completed;
			if (text != nullptr)
				asset_manager._import_status_pending = text;
			if (is_completed)
			{
				asset_manager._import_asset_paths_pending.resize(0);
				asset_manager._import_asset_paths_pending.reserve(imported_asset_paths.size);
				for (size_t i = 0; i < imported_asset_paths.size; ++i)
					asset_manager._import_asset_paths_pending.push_back(imported_asset_paths.data[i]);
			}
		}
		asset_manager._import_status_dirty.store(true, std::memory_order_release);
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
