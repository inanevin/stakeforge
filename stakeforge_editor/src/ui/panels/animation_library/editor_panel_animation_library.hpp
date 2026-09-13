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
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"

#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/animation_library_def.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_animation_library_edit_t;
	class editor_widget_animation_library_states_t;
	struct editor_dropdown_item_t;
	struct editor_asset_deletion_listener_tag_t;

	class editor_panel_animation_library_t final : public editor_panel_t
	{
	public:
		editor_panel_animation_library_t();
		~editor_panel_animation_library_t() override;
		editor_panel_animation_library_t(const editor_panel_animation_library_t& other)			   = delete;
		editor_panel_animation_library_t& operator=(const editor_panel_animation_library_t& other) = delete;

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

		void set_library(sid_t library_guid, const char* asset_name);
		void apply_edits(const animation_library_def_t& definition, u32 selected_layer, const bool* layer_expanded = nullptr, u32 selected_state = UINT32_MAX, u32 selected_clip = UINT32_MAX);
		void apply_selection(u32 selected_layer, u32 selected_state = UINT32_MAX, u32 selected_clip = UINT32_MAX);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		const animation_library_def_t& get_library_def() const
		{
			return _library;
		}

		u32 get_selected_layer() const
		{
			return _selected_layer;
		}

	private:
		friend class editor_command_animation_library_edit_t;
		friend class editor_widget_animation_library_states_t;
		struct layer_controls_t;

		void create_preview_world();
		void refresh_scene();
		void refresh_skeleton_reference();
		void refresh_skeleton_overlay();
		void apply_pane_splits();
		void refresh_skeleton_masks();
		void validate_layer_masks();
		void clear_layer_controls();
		void refresh_layer_controls();
		void refresh_layer_selection();
		void finish_edit();

		static void on_skeleton_edited(void* user_data);
		static void on_edit_begin(void* user_data);
		static void on_edit_submitted(void* user_data);
		static void on_layer_pressed(void* user_data);
		static void on_layer_fold_changed(bool folded, void* user_data);
		static void on_layer_add_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_layer_remove_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_refresh_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_asset_deletion(editor_asset_manager_t& manager, span_t<const sid_t> asset_ids, void* user_data);

	private:
		layer_controls_t*										 _layer_controls											 = nullptr;
		editor_widget_animation_library_states_t*				 _states_widget												 = nullptr;
		vector_t<string_t>										 _mask_names												 = {};
		vector_t<editor_dropdown_item_t>						 _mask_items												 = {};
		editor_scrollbar_t										 _left_scrollbar											 = {};
		editor_widget_button_t									 _add_layer_button											 = {};
		editor_widget_world_view_t								 _world_view												 = {};
		editor_widget_reference_t								 _skeleton_reference										 = {};
		editor_widget_button_t									 _save_changes_button										 = {};
		editor_scrollbar_t										 _right_scrollbar											 = {};
		editor_split_border_t									 _left_split_border											 = {};
		editor_split_border_t									 _right_split_border										 = {};
		string_t												 _asset_name												 = {};
		animation_library_def_t									 _library													 = {};
		resource_handle_t										 _skeleton_reference_value									 = NULL_RESOURCE_HANDLE;
		sid_t													 _library_guid												 = NULL_SID;
		editor_world_handle_t									 _world														 = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener									 = {};
		chunk_handle32_t										 _edit_previous_stream										 = {};
		u32														 _edit_previous_layer										 = UINT32_MAX;
		u32														 _selected_layer											 = UINT32_MAX;
		u32														 _selected_state											 = UINT32_MAX;
		u32														 _selected_clip												 = UINT32_MAX;
		u32														 _edit_previous_state										 = UINT32_MAX;
		u32														 _edit_previous_clip										 = UINT32_MAX;
		u32														 _layer_control_count										 = 0;
		ui::widget_id_t											 _left_pane													 = NULL_WIDGET;
		ui::widget_id_t											 _mid_pane													 = NULL_WIDGET;
		ui::widget_id_t											 _right_pane												 = NULL_WIDGET;
		ui::widget_id_t											 _missing_skeleton_frame									 = NULL_WIDGET;
		f32														 _left_split												 = 0.2f;
		f32														 _right_split												 = 0.75f;
		ui::widget_id_t											 _layer_list												 = NULL_WIDGET;
		ui::widget_id_t											 _right_content												 = NULL_WIDGET;
		bool													 _has_skeleton												 = false;
		bool													 _layer_expanded[MAX_ANIMATION_LIBRARY_LAYERS]				 = {};
		bool													 _edit_previous_layer_expanded[MAX_ANIMATION_LIBRARY_LAYERS] = {};
		bool													 _refresh_layers											 = false;
		bool													 _refresh_states											 = false;
		bool													 _refresh_skeleton											 = false;
	};
}
