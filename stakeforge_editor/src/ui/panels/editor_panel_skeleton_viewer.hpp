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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_skeleton_edit_t;
	class world_t;
	struct editor_asset_deletion_listener_tag_t;
	struct editor_gizmo_target_t;

	class editor_panel_skeleton_viewer_t final : public editor_panel_t
	{
	public:
		editor_panel_skeleton_viewer_t();
		~editor_panel_skeleton_viewer_t() override;
		editor_panel_skeleton_viewer_t(const editor_panel_skeleton_viewer_t&)			 = delete;
		editor_panel_skeleton_viewer_t& operator=(const editor_panel_skeleton_viewer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_skeleton(sid_t skeleton_guid, const char* asset_name);
		void apply_edits(vector_t<skeleton_slot_def_t>&& slots, vector_t<skeleton_mask_def_t>&& masks, u32 selected_joint, u32 selected_slot);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline skeleton_def_t& get_skeleton_def()
		{
			return _skeleton;
		}

		inline sid_t get_skeleton_guid() const
		{
			return _skeleton_guid;
		}

	private:
		friend class editor_command_skeleton_edit_t;

		struct joint_row_t
		{
			ui::widget_id_t root		   = NULL_WIDGET;
			ui::widget_id_t fold_icon	   = NULL_WIDGET;
			ui::widget_id_t fold_icon_text = NULL_WIDGET;
			ui::widget_id_t label		   = NULL_WIDGET;
			u32				joint_index	   = SKELETON_JOINT_NO_PARENT;
			u32				slot_index	   = UINT32_MAX;
			u32				first_child	   = SKELETON_JOINT_NO_PARENT;
			u32				next_sibling   = SKELETON_JOINT_NO_PARENT;
			u32				depth		   = 0;
			bool			expanded	   = true;
			bool			visible		   = true;
		};

		struct mask_item_t;

		struct slot_preview_t
		{
			resource_handle_t mesh	 = NULL_RESOURCE_HANDLE;
			entity_id_t		  entity = NULL_ENTITY_ID;
		};

		void init_animation_controls();
		void update_animation_player(bool reset);
		void refresh_animation_controls();
		void refresh_preview_animation_reference();
		void refresh_slot_entities();
		void update_slot_entity_transforms(world_t& world);
		void init_slot_fields();
		void refresh_slot_fields();
		void select_row(u32 row_index, bool additive = false);
		void refresh_mask_button();
		void refresh_joint_selection();
		void finish_mask_edit();
		void refresh_mask_backgrounds();
		void refresh_mask_items();
		void clear_mask_items();
		void add_slot();
		void duplicate_slot();
		void rename_slot();
		void delete_slot();
		void open_row_menu(const vec2f_t& pos);
		void refresh_joint_visibility();
		bool get_joint_world_transform(u32 joint_index, mat4x3_t& transform) const;
		bool get_slot_gizmo_target(editor_gizmo_target_t& target);
		bool begin_slot_gizmo();
		void update_slot_gizmo(const mat4x3_t& delta);
		void commit_slot_gizmo();
		void cancel_slot_gizmo();
		void init_joint_hierarchy();
		void refresh_joint_hierarchy();
		void create_joint_row(u32 joint_index);
		void update_joint_row_background(u32 joint_index);
		void toggle_joint_fold(u32 joint_index);

		void			create_preview_world();
		void			update_preview_environment();
		void			destroy_preview_world();
		void			create_display_entity();
		void			clear_display_entity();
		void			draw_skeleton(world_t& world) const;
		void			refresh_preview_mesh_reference();
		void			refresh_info();
		void			apply_pane_split();
		ui::widget_id_t append_property_value_row(const char* label);
		ui::widget_id_t append_value_label(ui::widget_id_t parent);

		static void on_preview_animation_edited(void* user_data);
		static void on_animation_play_pressed(bool toggled, void* user_data);
		static void on_animation_reset_pressed(bool toggled, void* user_data);
		static void on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_mask_edit_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_mask_remove_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_mask_activate_toggled(bool is_toggled, void* user_data);
		static void on_mask_name_edit_begin(void* user_data);
		static void on_mask_name_edit_submitted(void* user_data);
		static void on_make_mask_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_hierarchy_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_slot_rename_submitted(const char* value, void* user_data);
		static void on_row_menu_action(u16 action, void* user_data);
		static void on_slot_preview_mesh_edited(void* user_data);
		static void on_slot_fields_edited(void* user_data);
		static void on_slot_fields_edit_submitted(void* user_data);
		static void on_slot_fields_edit_begin(void* user_data);
		static void on_joint_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_joint_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_preview_mesh_edited(void* user_data);
		static void on_world_tick(world_t& world, f32 delta_time, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_widget_world_view_t _world_view					= {};
		editor_widget_reference_t  _preview_animation_reference = {};
		editor_icon_button_t	   _animation_play_button		= {};
		editor_icon_button_t	   _animation_reset_button		= {};
		editor_widget_button_t	   _save_changes_button			= {};
		editor_widget_button_t	   _make_mask_button			= {};
		vector_t<mask_item_t*>	   _mask_items					= {};
		vector_t<u32>			   _selected_joints				= {};
		editor_vec3_field_t		   _slot_position_field			= {};
		editor_vec3_field_t		   _slot_preview_scale_field	= {};
		editor_quat_field_t		   _slot_rotation_field			= {};
		editor_widget_reference_t  _slot_preview_mesh_reference = {};
		editor_scrollbar_t		   _right_scrollbar				= {};
		editor_widget_reference_t  _preview_mesh_reference		= {};
		editor_split_border_t	   _split_border				= {};
		skeleton_def_t			   _skeleton					= {};
		vector_t<joint_row_t>	   _joint_rows					= {};
		vector_t<slot_preview_t>   _slot_previews				= {};
		mat4x3_t				   _slot_initial_absolute		= mat4x3_t::identity;
		mat4x3_t				   _slot_parent_transform		= mat4x3_t::identity;
		quat_t					   _slot_parent_rotation		= quat_t::identity;
		quat_t					   _slot_initial_rotation		= quat_t::identity;
		vec3f_t					   _slot_initial_position		= vec3f_t::zero;
		vec3f_t					   _slot_initial_preview_scale	= vec3f_t::one;

		string_t												 _asset_name			  = {};
		string_t												 _joint_count_text		  = {};
		string_t												 _root_joint_text		  = {};
		editor_world_handle_t									 _world					  = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener = {};
		chunk_handle32_t										 _edit_previous_stream	  = {};
		resource_handle_t										 _preview_mesh			  = NULL_RESOURCE_HANDLE;
		resource_handle_t										 _preview_animation		  = NULL_RESOURCE_HANDLE;
		resource_handle_t										 _slot_preview_mesh		  = NULL_RESOURCE_HANDLE;
		sid_t													 _skeleton_guid			  = 0;
		entity_id_t												 _display_entity		  = NULL_ENTITY_ID;
		entity_id_t												 _environment_entity	  = NULL_ENTITY_ID;
		u32														 _root_joint_index		  = UINT32_MAX;
		u32														 _rename_slot_index		  = UINT32_MAX;
		u32														 _selected_slot_index	  = UINT32_MAX;
		u32														 _slot_generation		  = 0;
		u32														 _edit_previous_joint	  = SKELETON_JOINT_NO_PARENT;
		u32														 _edit_previous_slot	  = UINT32_MAX;
		u32														 _selected_joint_index	  = SKELETON_JOINT_NO_PARENT;
		ui::widget_id_t											 _joint_list_area		  = NULL_WIDGET;
		ui::widget_id_t											 _left_pane				  = NULL_WIDGET;
		ui::widget_id_t											 _right_pane			  = NULL_WIDGET;
		ui::widget_id_t											 _right_content			  = NULL_WIDGET;
		ui::widget_id_t											 _mask_list				  = NULL_WIDGET;
		u32														 _editing_mask			  = UINT32_MAX;
		u32														 _active_mask			  = UINT32_MAX;
		bool													 _mask_name_edit_active	  = false;
		ui::widget_id_t											 _joint_count_value		  = NULL_WIDGET;
		ui::widget_id_t											 _root_joint_value		  = NULL_WIDGET;
		f32														 _pane_split			  = 0.72f;
		bool													 _is_animation_playing	  = false;
		bool													 _slot_fields_edit_active = false;
		bool													 _slot_gizmo_active		  = false;
		bool													 _row_menu_open			  = false;
	};
}
