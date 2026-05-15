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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "widgets/editor_widget_width.hpp"
#include "widgets/editor_widgets_input_field.hpp"
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	using editor_color_field_changed_fn = void (*)(const vec4f_t& color, void* user_data);

	struct editor_color_field_config_t
	{
		editor_color_field_changed_fn on_changed = nullptr;
		void*						  user_data	 = nullptr;
		editor_widget_width_config_t  width		 = {};
		vec4f_t						  color		 = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	class editor_color_field_t final
	{
	public:
		editor_color_field_t()										 = default;
		~editor_color_field_t()										 = default;
		editor_color_field_t(const editor_color_field_t&)			 = delete;
		editor_color_field_t& operator=(const editor_color_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_field_config_t& config);
		void uninit();
		void set_color(const vec4f_t& color);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec4f_t& get_color() const
		{
			return _color;
		}

	private:
		void refresh_color();
		void refresh_text();

		static void on_text_changed(const char* value, void* user_data);

	private:
		ui::ui_context*				_ui			= nullptr;
		ui::widget_id_t				_root		= NULL_WIDGET;
		ui::widget_id_t				_swatch		= NULL_WIDGET;
		editor_color_field_config_t _config		= {};
		editor_input_field_t		_input		= {};
		vec4f_t						_color		= {1.0f, 1.0f, 1.0f, 1.0f};
		bool						_refreshing = false;
	};
}
