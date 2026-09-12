/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include "assets/editor_asset_database.hpp"
#include "assets/editor_asset_importer.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/bucketed_gen_pool.hpp>
#include <sfg/memory/gen_pool.hpp>

namespace sfg
{
	class editor_asset_manager_util_t;
	class editor_asset_manager_t;
	class ostream_t;
	struct editor_project_t;
	struct editor_asset_cook_state_tag_t;
	struct editor_asset_deletion_listener_tag_t;
	struct editor_file_change_t;
	struct editor_work_handle_tag_t;

	using editor_asset_deletion_listener_handle_t = pool_handle_t<u32, editor_asset_deletion_listener_tag_t>;
	using editor_asset_deletion_listener_fn		  = void (*)(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);

	struct editor_asset_deletion_listener_t
	{
		editor_asset_deletion_listener_fn fn		= nullptr;
		void*							  user_data = nullptr;
	};

	class editor_asset_manager_t final
	{
	public:
		editor_asset_manager_t()										 = default;
		~editor_asset_manager_t()										 = default;
		editor_asset_manager_t(const editor_asset_manager_t&)			 = delete;
		editor_asset_manager_t& operator=(const editor_asset_manager_t&) = delete;

		static inline editor_asset_manager_t& get()
		{
			return *s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void tick();
		void clear();
		void flush_asset_cook_jobs();
		void initialize_cooked_resource_tracking();
		void initialize_source_file_tracking();
		void initialize_script_file_tracking();
		void uninitialize_script_file_tracking();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void									register_descriptor(const editor_asset_descriptor_t& desc);
		void									import_assets(editor_asset_node_handle_t directory_node, const frame_vector_t<string_t>& paths, const frame_vector_t<editor_asset_import_options_t>& import_options);
		editor_asset_node_handle_t				add_folder_node(editor_asset_node_handle_t parent, const char* path);
		editor_asset_node_handle_t				add_path_node(editor_asset_node_handle_t parent, const char* path);
		editor_asset_node_handle_t				add_directory_tree(editor_asset_node_handle_t parent, const char* path);
		bool									reload_asset_node(editor_asset_node_handle_t node);
		void									sync_directory_from_disk(editor_asset_node_handle_t directory_node);
		void									sync_imported_asset_paths(editor_asset_node_handle_t directory_node, span_t<const string_t> paths);
		bool									delete_node_subtree(editor_asset_node_handle_t node);
		void									update_node_path(editor_asset_node_handle_t node, const char* new_path);
		void									move_node(editor_asset_node_handle_t node, editor_asset_node_handle_t new_parent, const char* new_path);
		void									notify_changed();
		void									process_file_changes(span_t<const editor_file_change_t> changes);
		bool									save_and_cook_embedded_asset_async(sid_t asset_id, const nlohmann::json& embedded_source);
		bool									save_and_cook_file_asset_blob_async(sid_t asset_id, const ostream_t& source);
		bool									save_and_cook_file_asset_options_async(sid_t asset_id, const nlohmann::json& cook_options);
		editor_asset_deletion_listener_handle_t add_asset_deletion_listener(editor_asset_deletion_listener_fn fn, void* user_data);
		void									remove_asset_deletion_listener(editor_asset_deletion_listener_handle_t handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const editor_asset_tree_t& get_asset_tree() const
		{
			return _database.get_asset_tree();
		}

		inline const hash_map_t<u64, editor_asset_t>& get_assets() const
		{
			return _database.get_assets();
		}

		inline const hash_map_t<editor_asset_type_e, editor_asset_descriptor_t>& get_asset_descriptors() const
		{
			return _asset_descriptors;
		}

		const editor_asset_descriptor_t* find_asset_descriptor(const string_t& extension) const;

		inline editor_asset_node_handle_t get_root_node() const
		{
			return _database.get_root_node();
		}

		inline const editor_asset_t* find_asset(u64 asset_id) const
		{
			return _database.find_asset(asset_id);
		}

		inline const editor_asset_node_t* find_asset_node(sid_t guid) const
		{
			return _database.find_asset_node_value(guid);
		}

		inline editor_asset_node_handle_t find_asset_node_handle(sid_t guid) const
		{
			return _database.find_asset_node(guid);
		}

		inline editor_asset_node_handle_t find_node_by_path(const char* path) const
		{
			return _database.find_node_by_path(path);
		}

		inline u32 get_generation() const
		{
			return _generation;
		}

		inline bool is_import_in_progress() const
		{
			return _import_in_progress;
		}

	private:
		struct asset_import_state_t
		{
			string_t								target_directory	 = {};
			vector_t<string_t>						paths				 = {};
			vector_t<string_t>						imported_asset_paths = {};
			vector_t<editor_asset_import_options_t> import_options		 = {};
		};

		struct asset_cook_state_t
		{
			editor_asset_t									  asset		   = {};
			string_t										  asset_path   = {};
			string_t										  display_name = {};
			pool_handle_t<u32, editor_asset_cook_state_tag_t> handle	   = {};
			u64												  revision	   = 0;
			bool											  save_asset   = false;
		};

		enum class cooked_resource_kind_e : u8
		{
			asset,
			thumbnail,
		};

		struct cooked_resource_tracking_state_t
		{
			string_t			   cache_path = {};
			sid_t				   asset_id	  = NULL_SID;
			cooked_resource_kind_e kind		  = cooked_resource_kind_e::asset;
		};

		struct source_file_tracking_state_t
		{
			string_t		full_path = {};
			vector_t<sid_t> asset_ids = {};
		};

		struct script_file_tracking_state_t
		{
			string_t full_path = {};
		};

		friend class editor_asset_manager_util_t;

		bool					   cook_asset_async(sid_t asset_id);
		void					   schedule_asset_cook(sid_t asset_id, editor_asset_t asset, const char* asset_path, const char* display_name, bool save_asset);
		bool					   asset_cook_worker(asset_cook_state_t& cook_state);
		void					   complete_asset_cook(pool_handle_t<u32, editor_work_handle_tag_t> work_handle, bool succeeded, asset_cook_state_t& cook_state);
		bool					   process_completed_shader_cook(const editor_asset_t& cooked_shader);
		void					   process_cooked_resource_change(sid_t resource_id);
		void					   process_changed_cooked_resources();
		void					   track_cooked_resource(sid_t resource_id, sid_t asset_id, cooked_resource_kind_e kind, bool report_existing_file);
		void					   untrack_cooked_resource(sid_t resource_id);
		void					   track_source_asset(const editor_asset_t& asset);
		void					   process_source_file_change(sid_t source_id);
		void					   process_changed_source_files();
		void					   untrack_source_asset(sid_t asset_id);
		void					   update_moved_source_paths(const char* old_path, const char* new_path, bool directory);
		void					   track_script_file(const char* path);
		bool					   process_script_file_change(sid_t script_id);
		bool					   update_moved_script_paths(const char* old_path, const char* new_path, bool directory);
		void					   notify_asset_deletion(span_t<const sid_t> asset_ids);
		editor_asset_node_handle_t find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const;
		editor_asset_node_handle_t get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name);

	private:
		editor_modal_progress_bar_t																_import_progress_modal = {};
		asset_import_state_t																	_import_state		   = {};
		editor_asset_database_t																	_database;
		gen_pool_t<editor_asset_deletion_listener_t, u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listeners;
		hash_map_t<editor_asset_type_e, editor_asset_descriptor_t>								_asset_descriptors;
		string_t																				_import_work_status_text = {};
		string_t																				_import_displayed_status = {};
		bucketed_gen_pool_t<asset_cook_state_t, editor_asset_cook_state_tag_t>					_asset_cook_states;
		hash_map_t<sid_t, u64>																	_latest_asset_cook_revisions;
		hash_map_t<sid_t, cooked_resource_tracking_state_t>										_cooked_resource_tracking_states;
		hash_map_t<sid_t, sid_t>																_cooked_path_to_resource;
		hash_map_t<sid_t, source_file_tracking_state_t>											_source_file_to_tracking;
		hash_map_t<sid_t, script_file_tracking_state_t>											_script_file_tracking_states;
		hash_map_t<sid_t, sid_t>																_asset_to_source_tracking;
		vector_t<sid_t>																			_changed_cooked_resources;
		vector_t<sid_t>																			_changed_source_assets;
		u64																						_next_asset_cook_revision	  = 1;
		pool_handle_t<u32, editor_work_handle_tag_t>											_last_asset_cook_work		  = {};
		pool_handle_t<u32, editor_work_handle_tag_t>											_import_work				  = {};
		editor_asset_node_handle_t																_import_target_directory_node = {};
		f32																						_import_work_progress		  = 0.0f;
		u32																						_generation					  = 0;
		u32																						_asset_cook_work_count		  = 0;
		bool																					_import_in_progress			  = false;
		bool																					_cooked_file_track_inited	  = false;
		bool																					_source_file_track_inited	  = false;
		bool																					_script_file_track_inited	  = false;
		bool																					_script_compile_requested	  = false;

		static inline editor_asset_manager_t* s_instance = nullptr;
	};
}
