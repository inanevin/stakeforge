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
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/animation_def.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_animation_events_edit_t;
	struct animation_def_t;
	struct editor_asset_deletion_listener_tag_t;

	namespace ui
	{
		class input_router_t;
		class paint_layer_t;
		class vg_canvas_t;
		enum class mouse_button_e : u8;
	}

	struct panel_animation_data_t
	{
		resource_handle_t target_mesh	   = NULL_RESOURCE_HANDLE;
		resource_handle_t target_skeleton  = NULL_RESOURCE_HANDLE;
		f32				  speed_multiplier = 1.0f;
	};

	SFG_DEFINE_TYPE_ID(panel_animation_data_t);

	struct panel_animation_data_reflection_t
	{
		panel_animation_data_reflection_t();
	};

	inline panel_animation_data_reflection_t g_reflect_panel_animation_data;

	class editor_panel_animation_t final : public editor_panel_t
	{
	public:
		editor_panel_animation_t();
		~editor_panel_animation_t() override;
		editor_panel_animation_t(const editor_panel_animation_t&)			 = delete;
		editor_panel_animation_t& operator=(const editor_panel_animation_t&) = delete;

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

		void set_animation(sid_t animation_guid, const char* asset_name);
		void apply_events(vector_t<animation_event_def_t>&& events, u32 selected_event);

		friend class editor_command_animation_events_edit_t;

	private:
		struct timeline_label_t
		{
			u32	 frame		 = 0;
			char text[12]	 = {};
			u8	 text_length = 0;
		};

		struct joint_row_t
		{
			editor_panel_animation_t* owner			 = nullptr;
			ui::widget_id_t			  left_root		 = NULL_WIDGET;
			ui::widget_id_t			  right_root	 = NULL_WIDGET;
			ui::widget_id_t			  fold_icon		 = NULL_WIDGET;
			u32						  first_child	 = SKELETON_JOINT_NO_PARENT;
			u32						  next_sibling	 = SKELETON_JOINT_NO_PARENT;
			u32						  depth			 = 0;
			u32						  keyframe_start = 0;
			u32						  keyframe_count = 0;
		};

		void create_preview_world();
		void destroy_preview_world();
		void create_display_entity();
		void clear_display_entity();
		void refresh_preview_skeleton();
		void refresh_joint_rows();
		void create_joint_row(u32 joint_index);
		void update_joint_row_background(u32 joint_index);
		void toggle_joint_fold(u32 joint_index);
		void select_event(u32 event_index);
		void refresh_event_fields();
		bool is_event_time_available(f32 time, u32 ignored_event = UINT32_MAX) const;
		u32	 get_event_at_position(const vec2f_t& position) const;
		void add_event();
		void duplicate_event();
		void delete_event();
		void open_event_menu(const vec2f_t& position, bool on_event);
		void clear_joint_rows();
		void rebuild_timeline(const animation_def_t* animation);
		void append_joint_keyframes(const animation_def_t& animation, u32 joint_index, joint_row_t& row);
		void set_timeline_cursor_from_position(const vec2f_t& position);
		void set_timeline_cursor_frame(u32 frame);
		void set_playing(bool is_playing);
		void update_animation_player();
		void refresh_data_reflection();
		void apply_pane_splits();

		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_data_edit_submitted(void* user_data);
		static void on_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_left_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_left_pane_bottom_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_left_pane_bottom_scroll_sync(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_play_pressed(bool toggled, void* user_data);
		static void on_reset_pressed(bool toggled, void* user_data);
		static void on_joint_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_joint_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_event_edit_begin(void* user_data);
		static void on_event_edited(void* user_data);
		static void on_event_edit_submitted(void* user_data);
		static void on_event_menu_action(u16 action, void* user_data);
		static void on_timeline_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_timeline_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_timeline_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_timeline_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void draw_timeline_toolbar(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static void draw_timeline_joint_row(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static void draw_timeline_cursor_body(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		editor_widget_world_view_t								 _world_view					 = {};
		editor_widget_reflection_t								 _data_reflection				 = {};
		editor_split_border_t									 _pane_split_border				 = {};
		editor_split_border_t									 _left_pane_split_border		 = {};
		editor_split_border_t									 _left_pane_bottom_split_border	 = {};
		editor_scrollbar_t										 _left_pane_bottom_scrollbar	 = {};
		editor_icon_button_t									 _play_button					 = {};
		editor_widget_reflection_t								 _event_reflection				 = {};
		editor_widget_button_t									 _save_changes_button			 = {};
		editor_icon_button_t									 _reset_button					 = {};
		animation_def_t											 _animation						 = {};
		animation_event_def_t									 _event_data					 = {};
		skeleton_def_t											 _preview_skeleton				 = {};
		vector_t<joint_row_t>									 _joint_rows					 = {};
		vector_t<timeline_label_t>								 _timeline_labels				 = {};
		vector_t<u32>											 _timeline_keyframes			 = {};
		vector_t<animation_event_def_t>							 _events						 = {};
		vector_t<u8>											 _joint_expanded				 = {};
		chunk_handle32_t										 _edit_previous_stream			 = {};
		string_t												 _asset_name					 = {};
		panel_animation_data_t									 _data							 = {};
		editor_world_handle_t									 _world							 = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener		 = {};
		sid_t													 _animation_guid				 = NULL_SID;
		entity_id_t												 _display_entity				 = NULL_ENTITY_ID;
		ui::widget_id_t											 _left_pane						 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_top					 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom				 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_left			 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_left_toolbar	 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_left_divider	 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_left_body	 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_right		 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_right_toolbar = NULL_WIDGET;
		ui::widget_id_t											 _timeline_toolbar_content		 = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_right_divider = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom_right_body	 = NULL_WIDGET;
		ui::widget_id_t											 _timeline_body_content			 = NULL_WIDGET;
		ui::widget_id_t											 _timeline_cursor_body			 = NULL_WIDGET;
		ui::widget_id_t											 _right_pane					 = NULL_WIDGET;
		entity_id_t												 _environment_entity			 = NULL_ENTITY_ID;
		ui::widget_id_t											 _duration_label				 = NULL_WIDGET;
		ui::widget_id_t											 _scrub_seconds_frame			 = NULL_WIDGET;
		ui::widget_id_t											 _scrub_seconds_label			 = NULL_WIDGET;
		f32														 _duration						 = 0.0f;
		f32														 _cursor_time					 = 0.0f;
		f32														 _scrub_label_opacity			 = 0.0f;
		u32														 _edit_previous_event			 = UINT32_MAX;
		u32														 _selected_event				 = UINT32_MAX;
		u32														 _selected_joint				 = SKELETON_JOINT_NO_PARENT;
		f32														 _timeline_content_width		 = 0.0f;
		f32														 _pane_split					 = 0.72f;
		f32														 _left_pane_split				 = 0.7f;
		f32														 _left_pane_bottom_split		 = 0.2f;
		u32														 _timeline_frame_count			 = 1;
		u32														 _timeline_cursor_frame			 = 0;
		char													 _timeline_cursor_label[12]		 = {};
		u8														 _timeline_cursor_label_length	 = 1;
		bool													 _event_menu_open				 = false;
		bool													 _refresh_event_fields			 = false;
		bool													 _scrubbing						 = false;
		bool													 _is_playing					 = false;
	};
}
