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
#include "assets/editor_asset_dependencies.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_default_asset_seeder.hpp"
#include "editor_mesh_generator.hpp"
#include "editor_project.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/mutex.hpp>
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
			vector_t<string_t>								imported_asset_paths;
			vector_t<editor_asset_import_options_t>			import_options;
			mutex_t											imported_asset_paths_mtx;
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
				state.callback(state.user_data, progress, text, false, {});
		}

		bool contains_guid(span_t<const sid_t> guids, sid_t guid)
		{
			for (size_t i = 0; i < guids.size; ++i)
			{
				if (guids.data[i] == guid)
					return true;
			}
			return false;
		}
	}

	void editor_asset_manager_util_t::build_asset_database(editor_asset_manager_t& asset_manager, const char* assets_dir)
	{
		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(assets_dir, entries);

		editor_asset_database_t& database = asset_manager._database;
		database.clear();
		database.reserve(static_cast<u32>(entries.size() + 1), static_cast<u32>(entries.size()));
		hash_map_t<u64, bool> found_assets;
		found_assets.reserve(entries.size());
		vector_t<sid_t> used_guids;
		used_guids.reserve(entries.size() * 2);

		const string_t					 root_name = file_system_t::get_last_folder_from_path(assets_dir);
		const string_t					 root_path = assets_dir;
		const editor_asset_node_handle_t root_node = database.emplace_node(editor_asset_node_t{.name = root_name, .full_path = root_path, .type = editor_asset_node_type_e::folder});
		database.set_root_node(root_node);

		vector_t<string_t> parts;
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(assets_dir, entry.path.c_str());
			parts.resize(0);
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t parent			 = root_node;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				parent = asset_manager.get_or_create_child_folder(parent, parts[i]);

			if (entry.type == file_system_entry_type_e::directory)
				continue;

			editor_asset_t asset = {};
			if (file_system_t::get_file_extension(entry.path) == "sfg_asset")
			{
				if (!editor_asset_io_t::read_asset(entry.path.c_str(), asset))
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
				if (contains_guid({.data = used_guids.data(), .size = used_guids.size()}, asset_id))
				{
					SFG_ERR("asset {0} has a guid that collides with another resource guid", entry.path.c_str());
					continue;
				}

				used_guids.push_back(asset.guid);
				bool		rewrite_asset		   = false;
				const sid_t builtin_thumbnail_guid = editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset.asset_type);
				if (builtin_thumbnail_guid != NULL_SID)
				{
					if (asset.thumbnail_guid != builtin_thumbnail_guid)
					{
						asset.thumbnail_guid = builtin_thumbnail_guid;
						rewrite_asset		 = true;
					}
				}
				else if (asset.thumbnail_guid == NULL_SID || contains_guid({.data = used_guids.data(), .size = used_guids.size()}, asset.thumbnail_guid) || asset.thumbnail_guid == asset.guid)
				{
					asset.thumbnail_guid = editor_asset_thumbnailer_t::make_thumbnail_guid(asset.asset_type, {.data = used_guids.data(), .size = used_guids.size()});
					rewrite_asset		 = true;
				}
				if (rewrite_asset && !editor_asset_io_t::write_asset(entry.path.c_str(), asset))
				{
					SFG_ERR("failed to update asset thumbnail guid {0}", entry.path.c_str());
					continue;
				}

				found_assets.emplace(asset_id, true);
				if (builtin_thumbnail_guid == NULL_SID)
					used_guids.push_back(asset.thumbnail_guid);
				database.upsert_asset(std::move(asset));

				const string_t					 name		= file_system_t::remove_extensions_from_path(parts.back());
				const editor_asset_node_handle_t asset_node = database.emplace_node(editor_asset_node_t{.asset_id = asset_id, .name = name, .full_path = entry.path, .type = editor_asset_node_type_e::asset});
				database.attach_node(parent, asset_node);
			}
			else
			{
				const editor_asset_node_type_e	 node_type = file_system_t::get_file_extension(entry.path) == "cs" ? editor_asset_node_type_e::script_file : editor_asset_node_type_e::file;
				const editor_asset_node_handle_t file_node = database.emplace_node(editor_asset_node_t{.name = parts.back(), .full_path = entry.path, .type = node_type});
				database.attach_node(parent, file_node);
			}
		}

		database.rebuild_indices();
		asset_manager._generation++;
	}

	void editor_asset_manager_util_t::ensure_integrity(editor_asset_manager_t& asset_manager)
	{
		const string_t&					 assets_path = editor_project_t::get()._runtime.assets_path;
		hash_map_t<u64, editor_asset_t>& assets		 = asset_manager._database.get_assets();
		for (auto& asset_pair : assets)
		{
			editor_asset_t& asset				  = asset_pair.second;
			asset.status						  = editor_asset_status_e::ok;
			const editor_asset_node_t* asset_node = asset_manager._database.find_asset_node_value(asset.guid);
			SFG_ASSERT(asset_node != nullptr);
			const auto	descriptor_it  = asset_manager._asset_descriptors.find(asset.asset_type);
			const char* asset_type_str = descriptor_it != asset_manager._asset_descriptors.end() && !descriptor_it->second.display_name.empty() ? descriptor_it->second.display_name.c_str() : "Unknown";

			if (asset.source_type == editor_asset_source_type_e::embedded && asset.embedded_source.empty())
			{
				asset.status = editor_asset_status_e::missing_embedded_data;
				SFG_WARN("asset {0}, {1}, {2} has missing embedded data", asset_node->full_path.c_str(), asset.guid, asset_type_str);
			}
			else if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				string_t source_path = file_system_t::get_absolute_path(assets_path.c_str());
				source_path += asset.source_relative;
				if (asset.source_relative.empty() || !file_system_t::exists(source_path.c_str()))
				{
					asset.status = editor_asset_status_e::missing_file_source;
					SFG_WARN("asset {0}, {1}, {2} has missing file source {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, asset.source_relative.c_str());
				}
			}

			vector_t<sid_t> dependencies;
			editor_asset_dependencies_t::fetch_dependencies(asset, dependencies);
			for (const sid_t dependency : dependencies)
			{
				if (assets.find(dependency) != assets.end())
					continue;

				if (asset.status == editor_asset_status_e::ok)
					asset.status = editor_asset_status_e::missing_dependency;
				SFG_WARN("asset {0}, {1}, {2} has missing dependency {3}", asset_node->full_path.c_str(), asset.guid, asset_type_str, dependency);
			}
		}
	}

	void editor_asset_manager_util_t::ensure_project_assets_async(editor_asset_manager_t& asset_manager, tf::Executor& executor, ensure_project_assets_progress_fn callback, void* user_data)
	{
		const string_t def_assets_path = editor_project_t::get()._runtime.default_assets_path;
		const string_t cache_path	   = editor_project_t::get()._runtime.cache_path;
		const string_t assets_path	   = editor_project_t::get()._runtime.assets_path;

		tf::Taskflow ensure_flow;
		ensure_flow.emplace([&asset_manager, def_assets_path, cache_path, assets_path, callback, user_data]() {
			const auto report_progress = [callback, user_data](f32 progress, const char* progress_text) {
				if (callback != nullptr)
					callback(user_data, progress, progress_text);
			};

			report_progress(0.0f, "Ensuring default assets");
			if (!file_system_t::exists(def_assets_path.c_str()))
				file_system_t::create_directory(def_assets_path.c_str());

			editor_default_asset_seeder_t::ensure(def_assets_path.c_str());

			report_progress(0.15f, "Scanning project assets");
			editor_asset_manager_util_t::build_asset_database(asset_manager, assets_path.c_str());

			report_progress(0.3f, "Cleaning asset cache");

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

					if (file_system_t::get_file_extension(entry.path) != "sfg_bin")
						continue;

					if (asset_manager.find_asset(guid) != nullptr)
						continue;

					bool found_thumbnail_asset = false;
					for (const auto& asset_pair : asset_manager._database.get_assets())
					{
						if (asset_pair.second.thumbnail_guid == guid)
						{
							found_thumbnail_asset = true;
							break;
						}
					}
					if (found_thumbnail_asset)
						continue;

					if (file_system_t::delete_file(entry.path.c_str()))
						SFG_ERR("failed deleting orphaned cache file {0}", entry.path.c_str());
				}
			}

			report_progress(0.45f, "Cooking project assets");
			// cook missing files.
			{
				hash_map_t<u64, editor_asset_t>& assets		 = asset_manager._database.get_assets();
				const size_t					 asset_count = assets.size();
				size_t							 index		 = 0;
				for (auto& asset_pair : assets)
				{
					editor_asset_t& asset	 = asset_pair.second;
					const f32		progress = asset_count != 0 ? 0.45f + (0.25f * static_cast<f32>(index) / static_cast<f32>(asset_count)) : 0.7f;
					report_progress(progress, "Cooking project assets");
					++index;

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

			report_progress(0.7f, "Preparing asset thumbnails");
			// ensure thumbs
			{
				hash_map_t<u64, editor_asset_t>& assets		 = asset_manager._database.get_assets();
				const size_t					 asset_count = assets.size();
				size_t							 index		 = 0;
				for (auto& asset_pair : assets)
				{
					editor_asset_t& asset	 = asset_pair.second;
					const f32		progress = asset_count != 0 ? 0.7f + (0.25f * static_cast<f32>(index) / static_cast<f32>(asset_count)) : 0.95f;
					report_progress(progress, "Preparing asset thumbnails");
					++index;

					const string_t thumb_cache = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);
					if (!file_system_t::exists(thumb_cache.c_str()))
						editor_asset_thumbnailer_t::generate_thumbnail(asset, nullptr);
				}
			}
		});
		executor.run(std::move(ensure_flow), [callback, user_data]() {
			if (callback != nullptr)
				callback(user_data, 1.0f, "Project assets ready");
		});
	}

	void editor_asset_manager_util_t::import_assets_async(const char* target_directory, span_t<const string_t> paths, span_t<const editor_asset_import_options_t> import_options, tf::Executor& executor, import_progress_fn callback, void* user_data)
	{
		import_progress_state_t* state = new import_progress_state_t();
		state->target_directory		   = target_directory;
		state->paths.reserve(paths.size);
		state->import_options.reserve(import_options.size);
		state->imported_asset_paths.reserve(paths.size);

		for (size_t i = 0; i < paths.size; ++i)
			state->paths.push_back(paths.data[i]);
		for (size_t i = 0; i < import_options.size; ++i)
			state->import_options.push_back(import_options.data[i]);

		state->callback	 = callback;
		state->user_data = user_data;

		const auto orm_options_it = std::find_if(state->import_options.begin(), state->import_options.end(), [](const editor_asset_import_options_t& options) { return options.type == editor_asset_import_type_e::orm_texture; });

		tf::Taskflow import_flow = {};
		if (orm_options_it != state->import_options.end())
		{
			state->total_count			   = 1;
			const size_t orm_options_index = static_cast<size_t>(orm_options_it - state->import_options.begin());
			import_flow.emplace([state, orm_options_index]() {
				vector_t<editor_asset_t>			 imported_assets	  = {};
				vector_t<string_t>					 imported_asset_paths = {};
				const editor_asset_import_options_t& orm_options		  = state->import_options[orm_options_index];

				if (state->callback != nullptr)
					state->callback(state->user_data, 0.0f, "Importing ORM texture", false, {});

				const editor_asset_import_context_t context = {
					.user_data	= state,
					.set_status = report_import_status,
				};

				const span_t<const string_t> source_paths = {
					.data = state->paths.data(),
					.size = state->paths.size(),
				};

				if (!editor_asset_importer_t::import_texture_orm(state->target_directory.c_str(), source_paths, orm_options.texture_cook_config, context, imported_assets, imported_asset_paths))
					SFG_ERR("failed importing ORM texture");
				else
				{
					LOCK_GUARD(state->imported_asset_paths_mtx);
					state->imported_asset_paths.reserve(state->imported_asset_paths.size() + imported_asset_paths.size());
					for (string_t& asset_path : imported_asset_paths)
						state->imported_asset_paths.push_back(std::move(asset_path));
				}

				state->imported_count.store(1, std::memory_order_relaxed);
				if (state->callback != nullptr)
					state->callback(state->user_data, 1.0f, "ORM texture import completed", false, {});
			});
		}
		else
		{
			state->total_count = static_cast<u32>(paths.size);
			for (size_t i = 0; i < state->paths.size(); ++i)
			{
				import_flow.emplace([state, i]() {
					vector_t<editor_asset_t> imported_assets	  = {};
					vector_t<string_t>		 imported_asset_paths = {};
					const string_t&			 path				  = state->paths[i];
					const f32				 progress			  = state->total_count != 0 ? static_cast<f32>(state->imported_count.load(std::memory_order_relaxed)) / static_cast<f32>(state->total_count) : 1.0f;

					if (state->callback != nullptr)
						state->callback(state->user_data, progress, path.c_str(), false, {});

					const span_t<const editor_asset_import_options_t> options = {
						.data = state->import_options.data(),
						.size = state->import_options.size(),
					};
					const editor_asset_import_context_t context = {
						.user_data	= state,
						.set_status = report_import_status,
					};

					if (!editor_asset_importer_t::import_asset(state->target_directory.c_str(), path.c_str(), options, context, imported_assets, imported_asset_paths))
						SFG_ERR("failed importing asset {0}", path.c_str());
					else
					{
						LOCK_GUARD(state->imported_asset_paths_mtx);
						state->imported_asset_paths.reserve(state->imported_asset_paths.size() + imported_asset_paths.size());
						for (string_t& asset_path : imported_asset_paths)
							state->imported_asset_paths.push_back(std::move(asset_path));
					}

					const u32 imported_count = state->imported_count.fetch_add(1, std::memory_order_relaxed) + 1;
					const f32 done_progress	 = state->total_count != 0 ? static_cast<f32>(imported_count) / static_cast<f32>(state->total_count) : 1.0f;
					if (state->callback != nullptr)
						state->callback(state->user_data, done_progress, path.c_str(), false, {});
				});
			}
		}

		executor.run(std::move(import_flow), [state]() {
			if (state->callback != nullptr)
				state->callback(state->user_data, 1.0f, "Import completed", true, {.data = state->imported_asset_paths.data(), .size = state->imported_asset_paths.size()});
			delete state;
		});
	}

	void editor_asset_manager_util_t::ensure_default_meshes()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		if (resource_manager.find_entry(DEFAULT_MESH_CUBE_GUID) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_cube({.size = vec3f_t::one}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CUBE_GUID, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(DEFAULT_MESH_PLANE_GUID) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_plane({.size = vec2f_t::one}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_PLANE_GUID, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(DEFAULT_MESH_SPHERE_GUID) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_sphere({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_SPHERE_GUID, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(DEFAULT_MESH_CYLINDER_GUID) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_cylinder({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CYLINDER_GUID, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(DEFAULT_MESH_CAPSULE_GUID) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_capsule({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(DEFAULT_MESH_CAPSULE_GUID, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(GIZMO_MESH_TRANSLATION) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_translation_gizmo({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(GIZMO_MESH_TRANSLATION, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(GIZMO_MESH_ROTATION) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_rotation_gizmo({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(GIZMO_MESH_ROTATION, resource_type_e::mesh, istream);
			}
		}

		if (resource_manager.find_entry(GIZMO_MESH_SCALE) == nullptr)
		{
			ostream_t stream = {};

			if (editor_mesh_generator_t::generate_scale_gizmo({}, stream))
			{
				istream_t istream = {};

				istream.open(stream.get_raw(), stream.get_size());
				resource_manager.load_resource_runtime(GIZMO_MESH_SCALE, resource_type_e::mesh, istream);
			}
		}
	}
}
