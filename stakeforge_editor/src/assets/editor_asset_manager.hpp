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

#pragma once

#include "assets/editor_asset_database.hpp"
#include "assets/editor_asset_importer.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/mutex.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/gen_pool.hpp>

namespace sfg
{
	class editor_asset_manager_util_t;
	class editor_asset_manager_t;
	struct editor_project_t;
	struct editor_asset_deletion_listener_tag_t;

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
		bool									save_and_cook_embedded_asset_async(sid_t asset_id, const nlohmann::json& embedded_source);
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
		struct asset_cook_state_t
		{
			editor_asset_t asset		= {};
			string_t	   asset_path	= {};
			string_t	   display_name = {};
			u64			   revision		= 0;
			bool		   save_asset	= false;
		};

		enum class cooked_resource_kind_e : u8
		{
			asset,
			thumbnail,
		};

		struct cooked_resource_tracking_state_t
		{
			string_t			   cache_path	 = {};
			sid_t				   asset_id		 = NULL_SID;
			u64					   last_modified = 0;
			cooked_resource_kind_e kind			 = cooked_resource_kind_e::asset;
		};

		struct source_file_tracking_state_t
		{
			string_t		full_path			   = {};
			vector_t<sid_t> asset_ids			   = {};
			u64				accepted_last_modified = 0;
			u64				pending_last_modified  = 0;
		};

		struct script_file_tracking_state_t
		{
			string_t full_path				= {};
			u64		 accepted_last_modified = 0;
			u64		 pending_last_modified	= 0;
		};

		friend class editor_asset_manager_util_t;

		static void				   on_import_progress(void* user_data, f32 progress, const char* text, bool is_completed, span_t<const string_t> imported_asset_paths);
		bool					   cook_asset_async(sid_t asset_id);
		void					   schedule_asset_cook(sid_t asset_id, editor_asset_t asset, const char* asset_path, const char* display_name, bool save_asset);
		void					   asset_cook_worker(sid_t asset_id);
		void					   scan_cooked_resources();
		void					   process_changed_cooked_resources();
		void					   track_cooked_resource(sid_t resource_id, sid_t asset_id, cooked_resource_kind_e kind, bool report_existing_file);
		void					   untrack_cooked_resource(sid_t resource_id);
		void					   track_source_asset(const editor_asset_t& asset);
		void					   scan_source_files();
		void					   process_changed_source_files();
		void					   untrack_source_asset(sid_t asset_id);
		void					   update_moved_source_paths(const char* old_path, const char* new_path, bool directory);
		void					   track_script_file(const char* path);
		void					   scan_script_files();
		bool					   update_moved_script_paths(const char* old_path, const char* new_path, bool directory);
		void					   notify_asset_deletion(span_t<const sid_t> asset_ids);
		editor_asset_node_handle_t find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const;
		editor_asset_node_handle_t get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name);

	private:
		editor_modal_progress_bar_t																_import_progress_modal = {};
		editor_asset_database_t																	_database;
		gen_pool_t<editor_asset_deletion_listener_t, u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listeners;
		hash_map_t<editor_asset_type_e, editor_asset_descriptor_t>								_asset_descriptors;
		string_t																				_import_status_pending;
		string_t																				_import_status_visible;
		vector_t<string_t>																		_import_asset_paths_pending;
		vector_t<string_t>																		_import_asset_paths_visible;
		mutex_t																					_import_status_mtx;
		hash_map_t<sid_t, asset_cook_state_t>													_asset_cook_states;
		hash_map_t<sid_t, cooked_resource_tracking_state_t>										_cooked_resource_tracking_states;
		hash_map_t<sid_t, source_file_tracking_state_t>											_source_file_to_tracking;
		hash_map_t<sid_t, script_file_tracking_state_t>											_script_file_tracking_states;
		hash_map_t<sid_t, sid_t>																_asset_to_source_tracking;
		vector_t<sid_t>																			_changed_cooked_resources;
		vector_t<sid_t>																			_changed_source_assets;
		mutex_t																					_asset_cook_mtx;
		u64																						_next_asset_cook_revision	  = 1;
		editor_asset_node_handle_t																_import_target_directory_node = {};
		atomic_t<u32>																			_asset_cook_worker_count	  = 0;
		f32																						_import_progress_pending	  = 0.0f;
		atomic_t<bool>																			_import_status_dirty		  = false;
		u32																						_generation					  = 0;
		u32																						_last_integrity_generation	  = 0;
		u16																						_cooked_resource_scan_ticks	  = 0;
		u16																						_source_file_scan_ticks		  = 0;
		bool																					_import_completed_pending	  = false;
		bool																					_import_in_progress			  = false;
		bool																					_cooked_file_track_inited	  = false;
		bool																					_source_file_track_inited	  = false;
		bool																					_script_file_track_inited	  = false;
		bool																					_script_compile_requested	  = false;

		static inline editor_asset_manager_t* s_instance = nullptr;
	};
}
