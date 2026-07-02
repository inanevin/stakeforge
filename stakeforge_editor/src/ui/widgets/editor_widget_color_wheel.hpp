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

#include "ui/widgets/editor_widget_input_field.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class axis_mode_e : u8;
	enum class flow_e : u8;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_color_wheel_data_changed_fn = void (*)(void* user_data);

	struct editor_color_wheel_field_t
	{
		span_t<color_t*> fields = {};
	};

	struct editor_color_wheel_config_t
	{
		editor_color_wheel_field_t		   field		   = {};
		editor_color_wheel_data_changed_fn on_data_changed = nullptr;
		void*							   user_data	   = nullptr;
	};

	class editor_widget_color_wheel_t final
	{
	public:
		editor_widget_color_wheel_t()											   = default;
		~editor_widget_color_wheel_t()											   = default;
		editor_widget_color_wheel_t(const editor_widget_color_wheel_t&)			   = delete;
		editor_widget_color_wheel_t& operator=(const editor_widget_color_wheel_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_wheel_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static f32 calculate_min_height();
		void	   set_visible(bool visible);
		void	   update_config(const editor_color_wheel_config_t& config);
		void	   update_field_data(editor_color_wheel_field_t field);
		void	   refresh_field_data();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static constexpr u32 PANE_COUNT			   = 6;
		static constexpr u32 TOP_RIGHT_FRAME_COUNT = 2;
		static constexpr u32 ROW_COUNT			   = 8;
		static constexpr u32 NUMBER_FIELD_COUNT	   = 7;
		static constexpr u32 HEX_TEXT_CAPACITY	   = 16;

		ui::widget_id_t make_pane(ui::widget_id_t parent, const char* debug_name, ui::flow_e flow, ui::axis_mode_e size_mode_x, ui::axis_mode_e size_mode_y, const vec2f_t& size_value, f32 child_spacing);
		void			make_top_left_frame(ui::widget_id_t parent);
		void			make_top_right_frame(ui::widget_id_t parent, u32 frame);
		void			make_number_row(ui::widget_id_t parent, u32 row, u32 field, const char* label);
		void			make_text_row(ui::widget_id_t parent, u32 row, const char* label);
		void			modify_field();
		void			update_displays(bool apply_wheel);
		void			apply_top_left_wheel(const vec2f_t& pos);
		void			apply_top_right_slider(ui::widget_id_t id, const vec2f_t& pos);
		void			set_widget_visible(ui::widget_id_t id, bool visible);

		static void on_rgba_changed(void* user_data);
		static void on_hsv_changed(void* user_data);
		static void on_hex_changed(void* user_data);
		static void on_top_left_frame_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_top_left_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_top_right_frame_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_top_right_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		ui::ui_context*				_ui										  = nullptr;
		ui::widget_id_t				_root									  = NULL_WIDGET;
		ui::widget_id_t				_panes[PANE_COUNT]						  = {};
		ui::widget_id_t				_top_left_frame							  = NULL_WIDGET;
		ui::widget_id_t				_top_left_handle						  = NULL_WIDGET;
		ui::widget_id_t				_top_right_frames[TOP_RIGHT_FRAME_COUNT]  = {};
		ui::widget_id_t				_top_right_handles[TOP_RIGHT_FRAME_COUNT] = {};
		ui::widget_id_t				_rows[ROW_COUNT]						  = {};
		ui::widget_id_t				_labels[ROW_COUNT]						  = {};
		editor_input_field_t		_inputs[ROW_COUNT]						  = {};
		vector_t<color_t*>			_fields									  = {};
		editor_color_wheel_config_t _config									  = {};
		color_t						_display_color							  = {};
		color_t						_top_right_drag_color					  = {};
		f32							_top_right_values[TOP_RIGHT_FRAME_COUNT]  = {};
		f32							_number_values[NUMBER_FIELD_COUNT]		  = {};
		char						_hex_value[HEX_TEXT_CAPACITY]			  = {};
	};
}
