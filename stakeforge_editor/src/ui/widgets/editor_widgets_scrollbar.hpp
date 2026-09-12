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
		f32	 align_scroll_value(f32 value) const;
		void set_scroll(axis_e axis, f32 value);
		void set_scroll_immediate(axis_e axis, f32 value);
		void update_wheel_scroll(f32 dt_seconds);
		void update_layout_outputs();
		void scroll_track_to(axis_state_t& axis, const vec2f_t& pos);

		static void on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_post_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_target_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void on_track_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_thumb_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		ui::ui_context*			  _ui					  = nullptr;
		editor_scrollbar_config_t _config				  = {};
		ui::widget_id_t			  _root					  = NULL_WIDGET;
		f32						  _scroll_value_y		  = 0.0f;
		f32						  _scroll_target_y		  = 0.0f;
		f32						  _scroll_velocity_y	  = 0.0f;
		axis_state_t			  _x					  = {};
		axis_state_t			  _y					  = {};
		bool					  _stick_y				  = false;
		bool					  _scroll_target_y_active = false;
	};
}
