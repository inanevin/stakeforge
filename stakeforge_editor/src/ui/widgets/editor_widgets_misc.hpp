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

	class editor_misc_widgets_t final
	{
	public:
		static ui::widget_id_t				add_spacer(ui::ui_context& ui, ui::widget_id_t parent, const vec2f_t& size);
		static editor_property_row_t		make_property_row(ui::ui_context& ui, ui::widget_id_t parent);
		static editor_property_row_t		make_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, bool sub_item = false, bool remove_button = false);
		static editor_vector_property_row_t make_vector_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, u32 item_count = 0, bool unfolded = false);
		static editor_window_buttons_t		add_window_buttons(
				 ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& alternative_frame_color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& icon_color, f32 icon_point_size);
	};
}
