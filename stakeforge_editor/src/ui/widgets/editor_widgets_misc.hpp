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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_property_row_t
	{
		ui::widget_id_t row			  = NULL_WIDGET;
		ui::widget_id_t left		  = NULL_WIDGET;
		ui::widget_id_t divider		  = NULL_WIDGET;
		ui::widget_id_t right		  = NULL_WIDGET;
		ui::widget_id_t label		  = NULL_WIDGET;
		ui::widget_id_t remove_button = NULL_WIDGET;
	};

	struct editor_vector_property_row_t
	{
		editor_property_row_t row			  = {};
		ui::widget_id_t		  dropdown_button = NULL_WIDGET;
		ui::widget_id_t		  dropdown_icon	  = NULL_WIDGET;
		ui::widget_id_t		  label			  = NULL_WIDGET;
		ui::widget_id_t		  count_label	  = NULL_WIDGET;
		ui::widget_id_t		  reset_button	  = NULL_WIDGET;
		ui::widget_id_t		  add_button	  = NULL_WIDGET;
	};

	struct editor_window_buttons_t
	{
		ui::widget_id_t minimize_frame = NULL_WIDGET;
		ui::widget_id_t maximize_frame = NULL_WIDGET;
		ui::widget_id_t close_frame	   = NULL_WIDGET;
		ui::widget_id_t minimize_icon  = NULL_WIDGET;
		ui::widget_id_t maximize_icon  = NULL_WIDGET;
		ui::widget_id_t close_icon	   = NULL_WIDGET;
	};

	struct editor_window_buttons_config_t
	{
		bool only_close = false;
	};

	class editor_misc_widgets_t final
	{
	public:
		static ui::widget_id_t				add_edit_blocker(ui::ui_context& ui, ui::widget_id_t parent);
		static ui::widget_id_t				add_spacer(ui::ui_context& ui, ui::widget_id_t parent, const vec2f_t& size);
		static ui::widget_id_t				make_section_label(ui::ui_context& ui, ui::widget_id_t parent, const char* text);
		static editor_property_row_t		make_property_row(ui::ui_context& ui, ui::widget_id_t parent, f32 indentation = 0.0f);
		static editor_property_row_t		make_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, bool sub_item = false, bool remove_button = false, f32 indentation = 0.0f);
		static editor_vector_property_row_t make_vector_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, u32 item_count = 0, bool unfolded = false, bool sub_item = false, f32 indentation = 0.0f);
		static editor_window_buttons_t		add_window_buttons(ui::ui_context&						 ui,
															   ui::widget_id_t						 parent,
															   const vec4f_t&						 frame_color,
															   const vec4f_t&						 alternative_frame_color,
															   const vec4f_t&						 hover_color,
															   const vec4f_t&						 press_color,
															   const vec4f_t&						 icon_color,
															   f32									 icon_point_size,
															   const editor_window_buttons_config_t& config = {});
	};
}
