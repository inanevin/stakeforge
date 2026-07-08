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

#include "assets/editor_asset.hpp"
#include "assets/editor_asset_importer.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/mutex.hpp>
#include <sfg/data/tree.hpp>
#include <sfg/io/assert.hpp>

namespace sfg
{
	struct editor_project_t;

	using editor_asset_tree_t = tree_t<editor_asset_node_t>;

	class editor_asset_manager_t final
	{
	public:
		editor_asset_manager_t()										 = default;
		~editor_asset_manager_t()										 = default;
		editor_asset_manager_t(const editor_asset_manager_t&)			 = delete;
		editor_asset_manager_t& operator=(const editor_asset_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void tick();
		void clear();
		void rescan(const string_t& assets_dir);
		void ensure_integrity();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void register_descriptor(const editor_asset_descriptor_t& desc);
		void ensure_project_assets_async();
		void ensure_thumbnails_loaded();
		void cook_assets(span_t<editor_asset_t*> assets);
		void import_assets(editor_asset_node_handle_t directory_node, const frame_vector_t<string_t>& paths, const frame_vector_t<editor_asset_import_options_t>& import_options);
		void set_import_status(const char* text);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const editor_asset_tree_t& get_asset_tree() const
		{
			return _asset_tree;
		}

		inline const hash_map_t<u64, editor_asset_t>& get_assets() const
		{
			return _assets;
		}

		inline const hash_map_t<editor_asset_type_e, editor_asset_descriptor_t>& get_asset_descriptors() const
		{
			return _asset_descriptors;
		}

		const editor_asset_descriptor_t* find_asset_descriptor(const string_t& extension) const;

		inline editor_asset_node_handle_t get_root_node() const
		{
			return _root_node;
		}

		inline const editor_asset_t* find_asset(u64 asset_id) const
		{
			const auto it = _assets.find(asset_id);
			return it != _assets.end() ? &it->second : nullptr;
		}

		inline u32 get_generation() const
		{
			return _generation;
		}

		inline bool is_ensure_project_assets_done() const
		{
			return _ensure_project_assets_done.load(std::memory_order_acquire);
		}

		static inline editor_asset_manager_t& get()
		{
			SFG_ASSERT(s_instance != nullptr);
			return *s_instance;
		}

	private:
		editor_asset_node_handle_t find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const;
		editor_asset_node_handle_t get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name);

	private:
		editor_modal_progress_bar_t								   _import_progress_modal = {};
		editor_asset_tree_t										   _asset_tree;
		hash_map_t<u64, editor_asset_t>							   _assets;
		hash_map_t<editor_asset_type_e, editor_asset_descriptor_t> _asset_descriptors;
		vector_t<string_t>										   _import_paths;
		vector_t<editor_asset_import_options_t>					   _import_options;
		vector_t<editor_asset_t>								   _cook_assets;
		string_t												   _import_status_pending;
		string_t												   _import_status_visible;
		mutex_t													   _import_status_mtx;
		atomic_t<bool>											   _ensure_project_assets_done = false;
		atomic_t<u32>											   _imported_count			   = 0;
		atomic_t<bool>											   _import_finished			   = false;
		atomic_t<bool>											   _import_status_dirty		   = false;
		editor_asset_node_handle_t								   _import_directory_node;
		editor_asset_node_handle_t								   _root_node;
		u32														   _generation		   = 0;
		u32														   _total_import_count = 0;
		bool													   _import_in_progress = false;

		static inline editor_asset_manager_t* s_instance = nullptr;
	};
}
