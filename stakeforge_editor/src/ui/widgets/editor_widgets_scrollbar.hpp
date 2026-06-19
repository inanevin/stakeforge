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
	enum editor_scrollbar_axis_flags_e : u8
	{
		editor_scrollbar_axis_x	 = 1 << 0,
		editor_scrollbar_axis_y	 = 1 << 1,
		editor_scrollbar_axis_xy = editor_scrollbar_axis_x | editor_scrollbar_axis_y,
	};

	struct editor_scrollbar_config_t
	{
		ui::widget_id_t target = NULL_WIDGET;
		u8				axes   = editor_scrollbar_axis_y;
	};

	class editor_scrollbar_t final
	{
	public:
		editor_scrollbar_t()									 = default;
		~editor_scrollbar_t()									 = default;
		editor_scrollbar_t(const editor_scrollbar_t&)			 = delete;
		editor_scrollbar_t& operator=(const editor_scrollbar_t&) = delete;

		void init(ui::ui_context& ui, const editor_scrollbar_config_t& config);
		void uninit();
		void scroll_to_end_y();
		void scroll_y(f32 delta);
		void set_scroll_y_immediate(f32 value);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		enum class axis_e : u8
		{
			x,
			y,
		};

		struct axis_state_t
		{
			editor_scrollbar_t* owner = nullptr;
			ui::widget_id_t		track = NULL_WIDGET;
			ui::widget_id_t		thumb = NULL_WIDGET;
			axis_e				axis  = axis_e::y;
		};

		void update_axis(axis_state_t& axis);
		void set_scroll(axis_e axis, f32 value);
		void set_scroll_immediate(axis_e axis, f32 value);
		void update_wheel_scroll(f32 dt_seconds);
		void scroll_track_to(axis_state_t& axis, const vec2f_t& pos);

		static void on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_target_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void on_track_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_thumb_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		ui::ui_context*			  _ui					  = nullptr;
		ui::widget_id_t			  _root					  = NULL_WIDGET;
		editor_scrollbar_config_t _config				  = {};
		axis_state_t			  _x					  = {};
		axis_state_t			  _y					  = {};
		f32						  _scroll_target_y		  = 0.0f;
		f32						  _scroll_velocity_y	  = 0.0f;
		bool					  _stick_y				  = false;
		bool					  _scroll_target_y_active = false;
	};
}
