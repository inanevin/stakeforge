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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_asset_manager_t;
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
		~editor_panel_animation_t() override								 = default;
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
			ui::widget_id_t			  left_divider	 = NULL_WIDGET;
			ui::widget_id_t			  right_root	 = NULL_WIDGET;
			ui::widget_id_t			  right_divider	 = NULL_WIDGET;
			u32						  keyframe_start = 0;
			u32						  keyframe_count = 0;
		};

		void create_preview_world();
		void destroy_preview_world();
		void create_display_entity();
		void clear_display_entity();
		void refresh_preview_skeleton();
		void refresh_joint_rows();
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
		skeleton_def_t											 _preview_skeleton				 = {};
		vector_t<joint_row_t>									 _joint_rows					 = {};
		vector_t<timeline_label_t>								 _timeline_labels				 = {};
		vector_t<u32>											 _timeline_keyframes			 = {};
		inplace_vector_t<resource_handle_t, 16>					 _preview_materials				 = {};
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
		f32														 _timeline_content_width		 = 0.0f;
		f32														 _pane_split					 = 0.72f;
		f32														 _left_pane_split				 = 0.7f;
		f32														 _left_pane_bottom_split		 = 0.2f;
		u32														 _timeline_frame_count			 = 1;
		u32														 _timeline_cursor_frame			 = 0;
		char													 _timeline_cursor_label[12]		 = {};
		u8														 _timeline_cursor_label_length	 = 1;
		bool													 _is_playing					 = false;
	};
}
