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
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_icon_button_clicked_fn = void (*)(bool toggled, void* user_data);

	struct editor_icon_button_config_t
	{
		vec4f_t						  frame_color		  = {0.0f, 0.0f, 0.0f, 0.0f};
		vec4f_t						  press_color		  = {0.0f, 0.0f, 0.0f, 0.0f};
		vec4f_t						  hover_color		  = {0.0f, 0.0f, 0.0f, 0.0f};
		vec4f_t						  frame_toggled_color = {0.0f, 0.0f, 0.0f, 0.0f};
		vec4f_t						  icon_color		  = {1.0f, 1.0f, 1.0f, 1.0f};
		vec4f_t						  disabled_color	  = {1.0f, 1.0f, 1.0f, 1.0f};
		const char*					  icon				  = nullptr;
		const char*					  toggled_icon		  = nullptr;
		const char*					  tooltip			  = nullptr;
		editor_icon_button_clicked_fn on_clicked		  = nullptr;
		void*						  user_data			  = nullptr;
		f32							  size				  = 0.0f;
		f32							  icon_size			  = 0.0f;
		bool						  toggle_enabled	  = false;
		bool						  toggled			  = false;
	};

	class editor_icon_button_t final
	{
	public:
		editor_icon_button_t()										 = default;
		~editor_icon_button_t()										 = default;
		editor_icon_button_t(const editor_icon_button_t&)			 = delete;
		editor_icon_button_t& operator=(const editor_icon_button_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_icon_button_config_t& config);
		void uninit();
		void set_toggled(bool toggled);
		void set_disabled(bool disabled);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_toggled() const
		{
			return _toggled;
		}

	private:
		void		refresh();
		const char* get_icon() const;

		static void on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		ui::ui_context*				_ui		  = nullptr;
		ui::widget_id_t				_root	  = NULL_WIDGET;
		ui::widget_id_t				_icon	  = NULL_WIDGET;
		editor_icon_button_config_t _config	  = {};
		bool						_toggled  = false;
		bool						_disabled = false;
	};
}
