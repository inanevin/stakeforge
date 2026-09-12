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

#include "ui/widgets/editor_widget_input_field.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/color.hpp>
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
	using editor_color_wheel_edit_begin_fn	 = void (*)(void* user_data);

	struct editor_color_wheel_field_t
	{
		span_t<color_t*> fields = {};
	};

	struct editor_color_wheel_config_t
	{
		editor_color_wheel_field_t		   field		   = {};
		editor_color_wheel_edit_begin_fn   edit_begin	   = nullptr;
		editor_color_wheel_data_changed_fn on_data_changed = nullptr;
		void*							   user_data	   = nullptr;
		bool							   hdr			   = false;
	};

	struct vec2f_t;

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

		static f32 calculate_min_height(bool hdr);
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
		static constexpr u32 TOP_RIGHT_FRAME_COUNT = 2;
		static constexpr u32 ROW_COUNT			   = 9;
		static constexpr u32 NUMBER_FIELD_COUNT	   = 8;
		static constexpr u32 EXPOSURE_FIELD		   = 7;
		static constexpr u32 EXPOSURE_ROW		   = 7;
		static constexpr u32 HEX_ROW			   = 8;
		static constexpr u32 HEX_TEXT_CAPACITY	   = 16;

		ui::widget_id_t make_pane(ui::widget_id_t parent, const char* debug_name, ui::flow_e flow, ui::axis_mode_e size_mode_x, ui::axis_mode_e size_mode_y, const vec2f_t& size_value, f32 child_spacing);
		void			make_top_left_frame(ui::widget_id_t parent);
		void			make_top_right_frame(ui::widget_id_t parent, u32 frame);
		void			make_number_row(ui::widget_id_t parent, u32 row, u32 field, const char* label);
		void			make_text_row(ui::widget_id_t parent, u32 row, const char* label);
		void			modify_field();
		void			update_displays(bool apply_wheel, bool preserve_focused_input);
		void			apply_top_left_wheel(const vec2f_t& pos);
		void			apply_top_right_slider(ui::widget_id_t id, const vec2f_t& pos);

		static void on_rgba_changed(void* user_data);
		static void on_hsv_changed(void* user_data);
		static void on_exposure_changed(void* user_data);
		static void on_hex_changed(void* user_data);
		static void on_input_edit_submitted(void* user_data);
		static void on_top_left_frame_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_top_left_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_top_right_frame_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_top_right_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_input_field_t		_inputs[ROW_COUNT]							  = {};
		editor_color_wheel_config_t _config										  = {};
		vector_t<color_t*>			_fields										  = {};
		ui::ui_context*				_ui											  = nullptr;
		color_t						_display_color								  = {};
		color_t						_top_right_base_colors[TOP_RIGHT_FRAME_COUNT] = {};
		ui::widget_id_t				_root										  = NULL_WIDGET;
		ui::widget_id_t				_top_left_frame								  = NULL_WIDGET;
		ui::widget_id_t				_top_left_handle							  = NULL_WIDGET;
		ui::widget_id_t				_exposure_row								  = NULL_WIDGET;
		ui::widget_id_t				_exposure_label								  = NULL_WIDGET;
		ui::widget_id_t				_top_right_frames[TOP_RIGHT_FRAME_COUNT]	  = {};
		ui::widget_id_t				_top_right_handles[TOP_RIGHT_FRAME_COUNT]	  = {};
		f32							_top_right_values[TOP_RIGHT_FRAME_COUNT]	  = {};
		f32							_number_values[NUMBER_FIELD_COUNT]			  = {};
		f32							_exposure									  = 0.0f;
		char						_hex_value[HEX_TEXT_CAPACITY]				  = {};
	};
}
