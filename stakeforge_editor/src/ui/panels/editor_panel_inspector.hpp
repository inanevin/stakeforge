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
#include "ui/widgets/editor_widgets_button.hpp"
#include "ui/widgets/editor_widgets_checkbox.hpp"
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_reflect_type.hpp"

namespace sfg
{
	class editor_panel_inspector_t final : public editor_panel_t
	{
	public:
		editor_panel_inspector_t();
		~editor_panel_inspector_t() override								 = default;
		editor_panel_inspector_t(const editor_panel_inspector_t&)			 = delete;
		editor_panel_inspector_t& operator=(const editor_panel_inspector_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

	private:
		static void on_text_changed(const char* value, void* user_data);
		static void on_number_changed(f32 value, void* user_data);
		static void on_checkbox_changed(bool checked, void* user_data);
		static void on_color_changed(const vec4f_t& value, void* user_data);
		static void on_vec2_changed(const vec2f_t& value, void* user_data);
		static void on_vec3_changed(const vec3f_t& value, void* user_data);
		static void on_vec4_changed(const vec4f_t& value, void* user_data);

	private:
		editor_input_field_t		 _text_input;
		editor_input_field_t		 _float_input;
		editor_input_field_t		 _int_input;
		editor_input_field_t		 _slider_input;
		editor_input_field_t		 _int_slider_input;
		editor_button_t				 _button;
		editor_checkbox_t			 _checkbox;
		editor_color_field_t		 _color_field;
		editor_vec2_field_t			 _vec2_field;
		editor_vec3_field_t			 _vec3_field;
		editor_vec4_field_t			 _vec4_field;
		editor_widget_fold_t		 _reflect_fold;
		editor_widget_reflect_type_t _reflect_type;
		ui::widget_id_t				 _column		   = NULL_WIDGET;
		f32							 _float_value	   = 12.5f;
		f32							 _int_value		   = 7.0f;
		f32							 _slider_value	   = 0.35f;
		f32							 _int_slider_value = 5.0f;
		bool						 _checkbox_value   = true;
		vec4f_t						 _color_value	   = {0.8f, 0.2f, 0.6f, 1.0f};
		vec2f_t						 _vec2_value	   = {1.0f, 2.0f};
		vec3f_t						 _vec3_value	   = {1.0f, 2.0f, 3.0f};
		vec4f_t						 _vec4_value	   = {1.0f, 2.0f, 3.0f, 4.0f};
	};
}
