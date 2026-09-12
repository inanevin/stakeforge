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

#include "ui/widgets/popups/editor_popup_world_view_settings.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "world/editor_world_handle.hpp"

namespace sfg
{
	enum class editor_transform_control_type_e : u8;

	class editor_widget_world_view_toolbars_t final
	{
	public:
		editor_widget_world_view_toolbars_t()													   = default;
		~editor_widget_world_view_toolbars_t()													   = default;
		editor_widget_world_view_toolbars_t(const editor_widget_world_view_toolbars_t&)			   = delete;
		editor_widget_world_view_toolbars_t& operator=(const editor_widget_world_view_toolbars_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_edit_world(editor_world_handle_t world, editor_world_edit_type_e edit_type);
		void set_transform_control_type(editor_transform_control_type_e type);
		void toggle_transform_locality();
		void toggle_transform_snapping();
		void refresh();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		struct transform_button_data_t
		{
			editor_widget_world_view_toolbars_t* toolbar = nullptr;
			editor_transform_control_type_e		 type	 = {};
		};

		void save_project_settings();
		void refresh_perf_metrics();

		static void on_transform_control_toggled(bool toggled, void* user_data);
		static void on_settings_pressed(bool toggled, void* user_data);
		static void on_settings_popup_install(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void on_settings_popup_uninstall(ui::ui_context& ui, void* user_data);
		static void on_locality_toggled(bool toggled, void* user_data);
		static void on_snapping_toggled(bool toggled, void* user_data);
		static void on_grid_toggled(bool toggled, void* user_data);
		static void on_bounding_boxes_toggled(bool toggled, void* user_data);
		static void on_physics_debug_toggled(bool toggled, void* user_data);
		static void on_shoot_rays_toggled(bool toggled, void* user_data);
		static void on_perf_metrics_toggled(bool toggled, void* user_data);
		static void on_perf_metrics_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		editor_popup_world_view_settings_t _settings_popup;
		editor_icon_button_t			   _transform_buttons[3];
		editor_icon_button_t			   _settings_button;
		editor_icon_button_t			   _perf_metrics_button;
		editor_icon_button_t			   _locality_button;
		editor_icon_button_t			   _snapping_button;
		editor_icon_button_t			   _grid_button;
		editor_icon_button_t			   _bounding_boxes_button;
		editor_icon_button_t			   _physics_debug_button;
		editor_icon_button_t			   _shoot_rays_button;
		transform_button_data_t			   _transform_button_data[3];
		ui::ui_context*					   _ui					   = nullptr;
		editor_world_handle_t			   _edit_world			   = {};
		ui::widget_id_t					   _root				   = NULL_WIDGET;
		ui::widget_id_t					   _left_column			   = NULL_WIDGET;
		ui::widget_id_t					   _right_column		   = NULL_WIDGET;
		ui::widget_id_t					   _top_left_row		   = NULL_WIDGET;
		ui::widget_id_t					   _top_right_row		   = NULL_WIDGET;
		ui::widget_id_t					   _global_frame		   = NULL_WIDGET;
		ui::widget_id_t					   _controls_frame		   = NULL_WIDGET;
		ui::widget_id_t					   _view_frame			   = NULL_WIDGET;
		ui::widget_id_t					   _physics_frame		   = NULL_WIDGET;
		ui::widget_id_t					   _perf_metrics_frame	   = NULL_WIDGET;
		ui::widget_id_t					   _perf_metrics_labels[5] = {};
		ui::widget_id_t					   _global_spacer		   = NULL_WIDGET;
		ui::widget_id_t					   _controls_spacer		   = NULL_WIDGET;
		ui::widget_id_t					   _view_spacer			   = NULL_WIDGET;
		editor_world_edit_type_e		   _edit_type			   = editor_world_edit_type_e::full_control;
		u8								   _perf_metrics_ticks	   = 0;
	};
}
