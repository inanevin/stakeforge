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

#include "world/editor_world_handle.hpp"
#include "world/editor_world_view_settings.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/gen_pool.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class editor_world_edit_context_t;
	class world_t;
	struct ecs_component_table_t;
	struct editor_world_folder_tag_t;
	struct editor_selection_listener_tag_t;

	using editor_world_folder_handle_t		 = pool_handle_t<u32, editor_world_folder_tag_t>;
	using editor_selection_listener_handle_t = pool_handle_t<u32, editor_selection_listener_tag_t>;
	using editor_selection_listener_fn		 = void (*)(editor_world_edit_context_t& context, void* user_data);

	enum class editor_outliner_item_type_e : u8
	{
		entity,
		folder,
	};

	enum class editor_transform_control_type_e : u8
	{
		invalid,
		move,
		rotate,
		scale,
	};

	enum class editor_transform_locality_e : u8
	{
		invalid,
		local,
		world,
	};

	enum class editor_transform_snapping_e : u8
	{
		invalid,
		none,
		default_,
	};

	struct editor_selection_listener_t
	{
		editor_selection_listener_fn fn		   = nullptr;
		void*						 user_data = nullptr;
	};

	struct editor_world_folder_t
	{
		vector_t<entity_guid_t>		 entity_guids  = {};
		string_t					 name_storage  = {};
		const char*					 name		   = nullptr;
		color_t						 color		   = {};
		u64							 guid		   = 0;
		editor_world_folder_handle_t parent_handle = {};
		bool						 folded		   = false;
	};

	struct editor_world_entity_metadata_t
	{
		editor_world_folder_handle_t folder_handle = {};
		entity_guid_t				 guid		   = NULL_ENTITY_GUID;
		bool						 folded		   = true;
	};

	struct editor_outliner_item_t
	{
		const char*					 name				  = nullptr;
		const char*					 type_icon			  = nullptr;
		entity_id_t					 entity				  = NULL_ENTITY_ID;
		entity_id_t					 parent_entity		  = NULL_ENTITY_ID;
		entity_guid_t				 entity_guid		  = NULL_ENTITY_GUID;
		editor_world_folder_handle_t folder_handle		  = {};
		color_t						 color				  = {};
		u16							 depth				  = 0;
		editor_outliner_item_type_e	 type				  = editor_outliner_item_type_e::entity;
		bool						 has_children		  = false;
		bool						 selected			  = false;
		bool						 disabled			  = false;
		bool						 has_prefab_reference = false;
	};

	class editor_world_edit_context_t final
	{
	public:
		editor_world_edit_context_t()												   = default;
		~editor_world_edit_context_t()												   = default;
		editor_world_edit_context_t(const editor_world_edit_context_t&)				   = delete;
		editor_world_edit_context_t& operator=(const editor_world_edit_context_t&)	   = delete;
		editor_world_edit_context_t(editor_world_edit_context_t&&) noexcept			   = default;
		editor_world_edit_context_t& operator=(editor_world_edit_context_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void set_world(editor_world_handle_t world);
		void clear();

		// -----------------------------------------------------------------------------
		// folders
		// -----------------------------------------------------------------------------

		editor_world_folder_handle_t create_folder(const char* name);
		editor_world_folder_handle_t create_folder_with_guid(const char* name, color_t color, bool folded, editor_world_folder_handle_t parent_handle, u64 guid);
		void						 destroy_folder(editor_world_folder_handle_t handle);
		void						 set_folder_name(editor_world_folder_handle_t handle, const char* name);
		void						 set_folder_color(editor_world_folder_handle_t handle, color_t color);
		void						 set_folder_folded(editor_world_folder_handle_t handle, bool folded);
		void						 set_folder_parent(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle);
		void						 assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids);
		void						 deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids);
		void						 write_folders_to_json(nlohmann::json& out_json) const;
		void						 read_folders_from_json(const nlohmann::json& in_json);

		// -----------------------------------------------------------------------------
		// outliner
		// -----------------------------------------------------------------------------

		void collect_outliner_items(const world_t& world);
		void set_entity_folded(entity_guid_t guid, bool folded);

		// -----------------------------------------------------------------------------
		// transform
		// -----------------------------------------------------------------------------

		inline void set_transform_control_type(editor_transform_control_type_e type)
		{
			_transform_control_type = type;
		}

		inline editor_transform_control_type_e get_transform_control_type() const
		{
			return _transform_control_type;
		}

		inline void set_transform_locality(editor_transform_locality_e locality)
		{
			_transform_locality = locality;
		}

		inline editor_transform_locality_e get_transform_locality() const
		{
			return _transform_locality;
		}

		inline void set_transform_snapping(editor_transform_snapping_e snapping)
		{
			_transform_snapping = snapping;
		}

		inline editor_transform_snapping_e get_transform_snapping() const
		{
			return _transform_snapping;
		}

		// -----------------------------------------------------------------------------
		// view
		// -----------------------------------------------------------------------------

		inline void set_grid_enabled(bool enabled)
		{
			_grid_enabled = enabled;
		}

		inline bool is_grid_enabled() const
		{
			return _grid_enabled;
		}

		inline void set_bounding_boxes_enabled(bool enabled)
		{
			_bounding_boxes_enabled = enabled;
		}

		inline bool is_bounding_boxes_enabled() const
		{
			return _bounding_boxes_enabled;
		}

		inline editor_world_view_settings_t& get_world_view_settings()
		{
			return _world_view_settings;
		}

		inline const editor_world_view_settings_t& get_world_view_settings() const
		{
			return _world_view_settings;
		}

		// -----------------------------------------------------------------------------
		// selection
		// -----------------------------------------------------------------------------

		void							   issue_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   clear_entity_selection();
		size_t							   collect_selected_root_entities(const world_t& world, span_t<entity_id_t> out_entities) const;
		editor_selection_listener_handle_t add_selection_listener(editor_selection_listener_fn fn, void* user_data);
		void							   remove_selection_listener(editor_selection_listener_handle_t handle);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		editor_world_folder_handle_t get_folder_handle(u64 guid) const;
		editor_world_folder_handle_t get_entity_folder(entity_guid_t guid) const;
		bool						 can_assign_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle) const;
		bool						 is_entity_expanded(entity_guid_t guid) const;
		void						 collect_folder_tree(editor_world_folder_handle_t handle, vector_t<editor_world_folder_handle_t>& out_handles) const;

		inline editor_world_folder_t& get_folder(editor_world_folder_handle_t handle)
		{
			return _folders.get(handle);
		}

		inline const editor_world_folder_t& get_folder(editor_world_folder_handle_t handle) const
		{
			return _folders.get(handle);
		}

		inline bool is_folder_valid(editor_world_folder_handle_t handle) const
		{
			return _folders.is_valid(handle);
		}

		inline span_t<editor_outliner_item_t> get_outliner_items()
		{
			return {.data = _outliner_items.data(), .size = _outliner_items.size()};
		}

		inline span_t<const editor_outliner_item_t> get_outliner_items() const
		{
			return {.data = _outliner_items.data(), .size = _outliner_items.size()};
		}

		inline span_t<const entity_id_t> get_selected_entities() const
		{
			return {.data = _selected_entities.data(), .size = _selected_entities.size()};
		}

		inline editor_world_handle_t get_world() const
		{
			return _world;
		}

		inline entity_id_t get_entity_anchor() const
		{
			return _entity_anchor;
		}

		inline u32 get_selection_generation() const
		{
			return _selection_generation;
		}

	private:
		struct outliner_component_tables_t
		{
			const ecs_component_table_t* hierarchy = nullptr;
			const ecs_component_table_t* name	   = nullptr;
			const ecs_component_table_t* disabled  = nullptr;
			const ecs_component_table_t* prefab	   = nullptr;
		};

		editor_world_entity_metadata_t&		  get_or_create_entity_metadata(entity_guid_t guid);
		editor_world_entity_metadata_t*		  find_entity_metadata(entity_guid_t guid);
		const editor_world_entity_metadata_t* find_entity_metadata(entity_guid_t guid) const;
		void								  append_folder_items(const world_t& world, const outliner_component_tables_t& tables, editor_world_folder_handle_t handle, u16 depth);
		void								  append_entity_items(const world_t& world, const outliner_component_tables_t& tables, entity_id_t id, u16 depth);
		bool								  is_entity_assigned(entity_guid_t guid) const;
		bool								  is_entity_selected(entity_id_t entity) const;
		void								  remove_entity_from_folders(entity_guid_t guid);
		void								  notify_selection_listeners();

	private:
		gen_pool_t<editor_selection_listener_t, u32, editor_selection_listener_tag_t> _selection_listeners;
		gen_pool_t<editor_world_folder_t, u32, editor_world_folder_tag_t>			  _folders;
		vector_t<editor_world_entity_metadata_t>									  _entity_metadata;
		vector_t<editor_outliner_item_t>											  _outliner_items;
		vector_t<entity_id_t>														  _selected_entities	  = {};
		editor_world_handle_t														  _world				  = {};
		entity_id_t																	  _entity_anchor		  = NULL_ENTITY_ID;
		u64																			  _next_guid			  = 1;
		u32																			  _selection_generation	  = 0;
		editor_transform_control_type_e												  _transform_control_type = editor_transform_control_type_e::move;
		editor_transform_locality_e													  _transform_locality	  = editor_transform_locality_e::local;
		editor_transform_snapping_e													  _transform_snapping	  = editor_transform_snapping_e::none;
		bool																		  _grid_enabled			  = false;
		bool																		  _bounding_boxes_enabled = false;
		editor_world_view_settings_t												  _world_view_settings	  = {};
	};
}
