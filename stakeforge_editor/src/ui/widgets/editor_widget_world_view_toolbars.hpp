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

		void set_edit_world(editor_world_handle_t world);
		void set_transform_control_type(editor_transform_control_type_e type);
		void toggle_transform_locality();
		void toggle_transform_snapping();

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

		void refresh();

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

	private:
		editor_popup_world_view_settings_t _settings_popup;
		editor_icon_button_t			   _transform_buttons[3];
		editor_icon_button_t			   _settings_button;
		editor_icon_button_t			   _locality_button;
		editor_icon_button_t			   _snapping_button;
		editor_icon_button_t			   _grid_button;
		editor_icon_button_t			   _bounding_boxes_button;
		editor_icon_button_t			   _physics_debug_button;
		editor_icon_button_t			   _shoot_rays_button;
		transform_button_data_t			   _transform_button_data[3];
		ui::ui_context*					   _ui			   = nullptr;
		editor_world_handle_t			   _edit_world	   = {};
		ui::widget_id_t					   _root		   = NULL_WIDGET;
		ui::widget_id_t					   _left_column	   = NULL_WIDGET;
		ui::widget_id_t					   _right_column   = NULL_WIDGET;
		ui::widget_id_t					   _top_left_row   = NULL_WIDGET;
		ui::widget_id_t					   _top_right_row  = NULL_WIDGET;
		ui::widget_id_t					   _global_frame   = NULL_WIDGET;
		ui::widget_id_t					   _controls_frame = NULL_WIDGET;
		ui::widget_id_t					   _view_frame	   = NULL_WIDGET;
		ui::widget_id_t					   _physics_frame  = NULL_WIDGET;
	};
}
