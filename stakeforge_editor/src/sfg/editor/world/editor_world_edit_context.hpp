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

#include "editor/editor_exports.hpp"

#include "editor/world/editor_world_handle.hpp"
#include "editor/world/editor_transform.hpp"
#include "editor/world/editor_world_view_settings.hpp"
#include <sfg/core/data/span.hpp>
#include <sfg/core/data/string.hpp>
#include <sfg/core/data/vector.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/math/color.hpp>
#include <sfg/core/memory/gen_pool.hpp>
#include <sfg/core/runtime/world/ecs_defs.hpp>
#include <sfg/core/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class editor_world_edit_context_t;
	class world_t;
	class ecs_component_table_t;
	struct editor_world_folder_tag_t;
	struct editor_selection_listener_tag_t;

	using editor_world_folder_handle_t		 = pool_handle_t<u32, editor_world_folder_tag_t>;
	using editor_selection_listener_handle_t = pool_handle_t<u32, editor_selection_listener_tag_t>;
	using editor_selection_listener_fn		 = void (*)(editor_world_edit_context_t& context, void* user_data);

	enum class editor_world_view_e : u8
	{
		invalid,
		lit,
		unlit,
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

	class SFG_EDITOR_API editor_world_edit_context_t final
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

		void init(editor_world_edit_type_e edit_type);
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

		inline void set_physics_debug_enabled(bool enabled)
		{
			_physics_debug_enabled = enabled;
		}

		inline bool is_physics_debug_enabled() const
		{
			return _physics_debug_enabled;
		}

		inline editor_world_view_settings_t& get_world_view_settings()
		{
			return _world_view_settings;
		}

		inline const editor_world_view_settings_t& get_world_view_settings() const
		{
			return _world_view_settings;
		}

		inline void set_world_view(editor_world_view_e view)
		{
			_world_view = view;
		}

		inline editor_world_view_e get_world_view() const
		{
			return _world_view;
		}

		// -----------------------------------------------------------------------------
		// editor camera
		// -----------------------------------------------------------------------------

		inline void set_editor_camera_entity(entity_id_t entity)
		{
			_editor_camera_entity = entity;
		}

		inline entity_id_t get_editor_camera_entity() const
		{
			return _editor_camera_entity;
		}

		// -----------------------------------------------------------------------------
		// selection
		// -----------------------------------------------------------------------------

		void							   issue_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   clear_entity_selection();
		size_t							   collect_mutable_entities(const world_t& world, span_t<const entity_id_t> entities, span_t<entity_id_t> out_entities) const;
		size_t							   collect_selected_mutable_root_entities(const world_t& world, span_t<entity_id_t> out_entities) const;
		editor_selection_listener_handle_t add_selection_listener(editor_selection_listener_fn fn, void* user_data);
		void							   remove_selection_listener(editor_selection_listener_handle_t handle);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		inline editor_world_edit_type_e get_edit_type() const
		{
			return _edit_type;
		}

		editor_world_folder_handle_t get_folder_handle(u64 guid) const;
		editor_world_folder_handle_t get_entity_folder(entity_guid_t guid) const;
		entity_id_t					 get_mutable_entity_anchor(const world_t& world) const;
		bool						 can_assign_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle) const;
		bool						 is_entity_child_insertion_allowed(const world_t& world, entity_id_t entity) const;
		bool						 is_entity_mutation_allowed(const world_t& world, entity_id_t entity) const;
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

		inline const gen_pool_t<editor_world_folder_t, u32, editor_world_folder_tag_t>& get_folders() const
		{
			return _folders;
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
		editor_world_entity_metadata_t&		  get_or_create_entity_metadata(entity_guid_t guid);
		editor_world_entity_metadata_t*		  find_entity_metadata(entity_guid_t guid);
		const editor_world_entity_metadata_t* find_entity_metadata(entity_guid_t guid) const;
		bool								  is_entity_mutation_allowed(const ecs_component_table_t& prefab_table, entity_id_t entity) const;
		bool								  is_selected_mutable_root_entity(const world_t& world, const ecs_component_table_t& prefab_table, entity_id_t entity) const;
		void								  remove_entity_from_folders(entity_guid_t guid);
		void								  notify_selection_listeners();

	private:
		gen_pool_t<editor_selection_listener_t, u32, editor_selection_listener_tag_t> _selection_listeners;
		gen_pool_t<editor_world_folder_t, u32, editor_world_folder_tag_t>			  _folders;
		vector_t<editor_world_entity_metadata_t>									  _entity_metadata;
		vector_t<entity_id_t>														  _selected_entities	  = {};
		editor_world_view_settings_t												  _world_view_settings	  = {};
		editor_world_handle_t														  _world				  = {};
		entity_id_t																	  _entity_anchor		  = NULL_ENTITY_ID;
		entity_id_t																	  _editor_camera_entity	  = NULL_ENTITY_ID;
		u64																			  _next_guid			  = 1;
		u32																			  _selection_generation	  = 0;
		editor_transform_control_type_e												  _transform_control_type = editor_transform_control_type_e::move;
		editor_transform_locality_e													  _transform_locality	  = editor_transform_locality_e::local;
		editor_transform_snapping_e													  _transform_snapping	  = editor_transform_snapping_e::none;
		editor_world_view_e															  _world_view			  = editor_world_view_e::lit;
		bool																		  _grid_enabled			  = false;
		bool																		  _bounding_boxes_enabled = false;
		bool																		  _physics_debug_enabled  = false;
		editor_world_edit_type_e													  _edit_type			  = editor_world_edit_type_e::full_control;
	};
}
