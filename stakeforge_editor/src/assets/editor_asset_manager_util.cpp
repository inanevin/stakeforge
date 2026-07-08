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

#include "assets/editor_asset_manager_util.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_default_asset_seeder.hpp"
#include "editor_mesh_generator.hpp"
#include "editor_project.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/resource_type.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>

namespace sfg
{
	namespace
	{
		struct import_progress_state_t
		{
			string_t										target_directory;
			vector_t<string_t>								paths;
			vector_t<editor_asset_import_options_t>			import_options;
			editor_asset_manager_util_t::import_progress_fn callback	   = nullptr;
			void*											user_data	   = nullptr;
			atomic_t<u32>									imported_count = 0;
			u32												total_count	   = 0;
		};

		void report_import_status(void* user_data, const char* text)
		{
			import_progress_state_t& state	  = *static_cast<import_progress_state_t*>(user_data);
			const u32				 count	  = state.imported_count.load(std::memory_order_relaxed);
			const f32				 progress = state.total_count != 0 ? static_cast<f32>(count) / static_cast<f32>(state.total_count) : 1.0f;
			if (state.callback != nullptr)
				state.callback(state.user_data, progress, text, false);
		}
	}

	void editor_asset_manager_util_t::rescan(editor_asset_manager_t& asset_manager, const char* assets_dir)
	{
		SFG_ASSERT(assets_dir != nullptr);
		SFG_ASSERT(assets_dir[0] != '\0');

		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(assets_dir, entries);

		asset_manager._asset_tree.resize_zero();
		asset_manager._asset_tree.reserve(static_cast<u32>(entries.size() + 1));
		hash_map_t<u64, editor_asset_t> found_assets;
		found_assets.reserve(entries.size());

		const string_t root_name = file_system_t::get_last_folder_from_path(assets_dir);
		const string_t root_path = assets_dir;
		asset_manager._root_node = asset_manager._asset_tree.emplace(editor_asset_node_t{.name = root_name, .full_path = root_path, .type = editor_asset_node_type_e::folder});

		vector_t<string_t> parts;
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(assets_dir, entry.path.c_str());
			parts.resize(0);
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t parent			 = asset_manager._root_node;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				parent = asset_manager.get_or_create_child_folder(parent, parts[i]);

			if (entry.type == file_system_entry_type_e::directory)
				continue;

			editor_asset_t asset = {};
			if (file_system_t::get_file_extension(entry.path) == "sfg_asset")
			{
				if (!editor_asset_util_t::read_asset(entry.path.c_str(), asset))
					continue;

				const u64 asset_id = asset.guid;
				if (asset_id == NULL_SID)
				{
					SFG_ERR("asset {0} has an invalid guid", entry.path.c_str());
					continue;
				}
				if (found_assets.find(asset_id) != found_assets.end())
				{
					SFG_ERR("asset {0} has a duplicate guid", entry.path.c_str());
					continue;
				}

				found_assets.emplace(asset_id, std::move(asset));

				const string_t					 name		= file_system_t::remove_extensions_from_path(parts.back());
				const editor_asset_node_handle_t asset_node = asset_manager._asset_tree.emplace(editor_asset_node_t{.asset_id = asset_id, .name = name, .full_path = entry.path, .type = editor_asset_node_type_e::asset});
				asset_manager._asset_tree.attach(parent, asset_node);
			}
			else
			{
				const editor_asset_node_handle_t file_node = asset_manager._asset_tree.emplace(editor_asset_node_t{.name = parts.back(), .full_path = entry.path, .type = editor_asset_node_type_e::file});
				asset_manager._asset_tree.attach(parent, file_node);
			}
		}

		for (auto it = asset_manager._assets.begin(); it != asset_manager._assets.end();)
		{
			const u64 asset_id = it->first;
			++it;
			if (found_assets.find(asset_id) == found_assets.end())
				asset_manager._assets.erase(asset_id);
		}
		for (auto& found : found_assets)
			asset_manager._assets[found.first] = std::move(found.second);

		asset_manager._generation++;
	}

	void editor_asset_manager_util_t::ensure_integrity(editor_asset_manager_t& asset_manager)
	{
		const string_t&								assets_path = editor_project_t::get()._runtime.assets_path;
		hash_map_t<u64, const editor_asset_node_t*> asset_nodes;
		asset_nodes.reserve(asset_manager._assets.size());
		for (auto it = asset_manager._asset_tree.begin_handle(); it != asset_manager._asset_tree.end_handle(); ++it)
		{
			const editor_asset_node_t& node = asset_manager._asset_tree.value(*it);
			if (node.type == editor_asset_node_type_e::asset)
				asset_nodes[node.asset_id] = &node;
		}

		for (auto& asset_pair : asset_manager._assets)
		{
			editor_asset_t& asset	 = asset_pair.second;
			asset.status			 = editor_asset_status_e::ok;
			const auto asset_node_it = asset_nodes.find(asset.guid);
			SFG_ASSERT(asset_node_it != asset_nodes.end());
			const editor_asset_node_t* asset_node	  = asset_node_it->second;
			const auto				   descriptor_it  = asset_manager._asset_descriptors.find(asset.asset_type);
			const char*				   asset_type_str = descriptor_it != asset_manager._asset_descriptors.end() && !descriptor_it->second.display_name.empty() ? descriptor_it->second.display_name.c_str() : "Unknown";

			if (asset.source_type == editor_asset_source_type_e::embedded && asset.embedded_source.empty())
			{
				asset.status = editor_asset_status_e::missing_embedded_data;
				SFG_ERR("asset {0}, {1}, {2} has missing embedded data", asset_node->full_path.c_str(), asset.guid, asset_type_str);
			}
			else if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				string_t source_path = file_system_t::get_absolute_path(assets_path.c_str());
				source_path += asset.source_relative;
				if (asset.source_relative.empty() || !file_system_t::exists(source_path.c_str()))
				{
					asset.status = editor_asset_status_e::missing_file_source;
					SFG_ERR("asset {0}, {1}, {2} has missing file source {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, asset.source_relative.c_str());
				}
			}

			vector_t<sid_t> dependencies;
			editor_asset_util_t::fetch_dependencies(asset, dependencies);
			for (const sid_t dependency : dependencies)
			{
				if (asset_manager._assets.find(dependency) != asset_manager._assets.end())
					continue;

				if (asset.status == editor_asset_status_e::ok)
					asset.status = editor_asset_status_e::missing_dependency;
				SFG_ERR("asset {0}, {1}, {2} has missing dependency {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, dependency);
			}
		}
	}

	void editor_asset_manager_util_t::ensure_project_assets_async(editor_asset_manager_t& asset_manager, tf::Executor& executor, on_complete_fn on_complete, void* user_data)
	{
		const string_t def_assets_path = editor_project_t::get()._runtime.default_assets_path;
		const string_t cache_path	   = editor_project_t::get()._runtime.cache_path;
		const string_t assets_path	   = editor_project_t::get()._runtime.assets_path;
		SFG_ASSERT(!def_assets_path.empty());
		SFG_ASSERT(!cache_path.empty());
		SFG_ASSERT(!assets_path.empty());

		tf::Taskflow ensure_flow;
		ensure_flow.emplace([&asset_manager, def_assets_path, cache_path, assets_path]() {
			if (!file_system_t::exists(def_assets_path.c_str()))
				file_system_t::create_directory(def_assets_path.c_str());

			editor_default_asset_seeder_t::ensure(def_assets_path.c_str());

			editor_asset_manager_util_t::rescan(asset_manager, assets_path.c_str());

			// clean stale binarires
			{
				SFG_ASSERT(!cache_path.empty());

				vector_t<file_system_entry_t> entries;
				file_system_t::get_entries_recursive(cache_path.c_str(), entries);
				for (const file_system_entry_t& entry : entries)
				{
					if (entry.type != file_system_entry_type_e::file)
						continue;

					const string_t guid_text = file_system_t::get_filename_from_path(entry.path);
					u64			   guid		 = 0;
					string_util::to_big_uint(guid_text, guid);

					if (file_system_t::get_file_extension(entry.path) == "sfg_thumb_bin")
					{
						bool found_thumbnail_asset = false;
						for (const auto& asset_pair : asset_manager._assets)
						{
							if (editor_asset_thumbnailer_t::get_thumbnail_guid(asset_pair.second.guid) == guid)
							{
								found_thumbnail_asset = true;
								break;
							}
						}

						if (found_thumbnail_asset)
							continue;
					}
					else if (asset_manager.find_asset(guid) != nullptr)
						continue;

					if (file_system_t::delete_file(entry.path.c_str()))
						SFG_ERR("failed deleting orphaned cache file {0}", entry.path.c_str());
				}
			}

			// cook missing files.
			{
				for (auto& asset_pair : asset_manager._assets)
				{
					editor_asset_t& asset = asset_pair.second;
					SFG_ASSERT(asset_manager._asset_descriptors.find(asset.asset_type) != asset_manager._asset_descriptors.end());
					if (asset.status != editor_asset_status_e::ok)
						continue;

					if (!editor_asset_cooker_t::is_cookable(asset.asset_type))
						continue;

					if (editor_asset_cooker_t::is_asset_cooked(asset))
						continue;

					if (!editor_asset_cooker_t::cook_asset(asset))
					{
						SFG_ERR("failed cooking asset {0}", asset.guid);
						continue;
					}
				}
			}

			// ensure thumbs
			{
				for (auto& asset_pair : asset_manager._assets)
				{
					editor_asset_t& asset = asset_pair.second;
					editor_asset_thumbnailer_t::ensure(asset);
				}
			}
		});
		executor.run(std::move(ensure_flow), [on_complete, user_data]() {
			if (on_complete != nullptr)
				on_complete(user_data);
		});
	}

	void editor_asset_manager_util_t::import_assets_async(const char* target_directory, span_t<const string_t> paths, span_t<const editor_asset_import_options_t> import_options, tf::Executor& executor, import_progress_fn callback, void* user_data)
	{
		SFG_ASSERT(target_directory != nullptr);
		SFG_ASSERT(target_directory[0] != '\0');
		SFG_ASSERT(paths.data != nullptr);
		SFG_ASSERT(paths.size != 0);
		SFG_ASSERT(import_options.data != nullptr);
		SFG_ASSERT(import_options.size != 0);

		import_progress_state_t* state = new import_progress_state_t();
		state->target_directory		   = target_directory;
		state->paths.reserve(paths.size);
		state->import_options.reserve(import_options.size);
		for (size_t i = 0; i < paths.size; ++i)
			state->paths.push_back(paths.data[i]);
		for (size_t i = 0; i < import_options.size; ++i)
			state->import_options.push_back(import_options.data[i]);
		state->callback	   = callback;
		state->user_data   = user_data;
		state->total_count = static_cast<u32>(paths.size);

		tf::Taskflow import_flow;
		for (size_t i = 0; i < state->paths.size(); ++i)
		{
			import_flow.emplace([state, i]() {
				vector_t<editor_asset_t> imported_assets;
				const string_t&			 path	  = state->paths[i];
				const f32				 progress = state->total_count != 0 ? static_cast<f32>(state->imported_count.load(std::memory_order_relaxed)) / static_cast<f32>(state->total_count) : 1.0f;
				if (state->callback != nullptr)
					state->callback(state->user_data, progress, path.c_str(), false);

				const span_t<const editor_asset_import_options_t> options = {
					.data = state->import_options.data(),
					.size = state->import_options.size(),
				};
				const editor_asset_import_context_t context = {
					.user_data	= state,
					.set_status = report_import_status,
				};

				if (!editor_asset_importer_t::import_asset(state->target_directory.c_str(), path.c_str(), options, context, imported_assets))
					SFG_ERR("failed importing asset {0}", path.c_str());

				for (const editor_asset_t& asset : imported_assets)
					editor_asset_thumbnailer_t::ensure(asset, nullptr, true);

				const u32 imported_count = state->imported_count.fetch_add(1, std::memory_order_relaxed) + 1;
				const f32 done_progress	 = state->total_count != 0 ? static_cast<f32>(imported_count) / static_cast<f32>(state->total_count) : 1.0f;
				if (state->callback != nullptr)
					state->callback(state->user_data, done_progress, path.c_str(), false);
			});
		}
		executor.run(std::move(import_flow), [state]() {
			if (state->callback != nullptr)
				state->callback(state->user_data, 1.0f, "Import completed", true);
			delete state;
		});
	}

	void editor_asset_manager_util_t::ensure_thumbnails_loaded(editor_asset_manager_t& asset_manager)
	{
		for (auto& asset_pair : asset_manager._assets)
		{
			editor_asset_t& asset = asset_pair.second;
			editor_asset_thumbnailer_t::ensure_thumbnail_loaded(asset);
		}
	}

	void editor_asset_manager_util_t::ensure_default_meshes()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		if (resource_manager.find_entry(DEFAULT_MESH_CUBE_GUID) == nullptr)
		{
			ostream_t stream;
			if (editor_mesh_generator_t::generate_cube({.size = vec3f_t::one}, stream))
			{
				istream_t istream;
				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CUBE_GUID, resource_type_e::mesh, istream);
			}
		}
		if (resource_manager.find_entry(DEFAULT_MESH_SPHERE_GUID) == nullptr)
		{
			ostream_t stream;
			if (editor_mesh_generator_t::generate_sphere({}, stream))
			{
				istream_t istream;
				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_SPHERE_GUID, resource_type_e::mesh, istream);
			}
		}
		if (resource_manager.find_entry(DEFAULT_MESH_CYLINDER_GUID) == nullptr)
		{
			ostream_t stream;
			if (editor_mesh_generator_t::generate_cylinder({}, stream))
			{
				istream_t istream;
				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CYLINDER_GUID, resource_type_e::mesh, istream);
			}
		}
		if (resource_manager.find_entry(DEFAULT_MESH_CAPSULE_GUID) == nullptr)
		{
			ostream_t stream;
			if (editor_mesh_generator_t::generate_capsule({}, stream))
			{
				istream_t istream;
				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CAPSULE_GUID, resource_type_e::mesh, istream);
			}
		}
	}
}
