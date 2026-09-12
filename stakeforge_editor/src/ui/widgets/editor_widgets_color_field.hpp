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

#include "ui/widgets/editor_widget_width.hpp"
#include "ui/widgets/editor_widgets_common.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/color.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_popup_color_wheel_t;

	struct editor_color_field_field_t
	{
		span_t<color_t*> fields		   = {};
		span_t<f32*>	 float4_fields = {};
	};

	struct editor_color_field_config_t
	{
		editor_color_field_field_t	 field	   = {};
		editor_widget_width_config_t width	   = {};
		editor_widget_callbacks_t	 callbacks = {};
		bool						 hdr	   = false;
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
		void set_color(const color_t& color);
		void update_field_data(editor_color_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const color_t& get_color() const
		{
			return _color;
		}

	private:
		void refresh_color();
		void modify_field();
		void begin_edit();
		void submit_edit();

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_color_wheel_popup_install(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void on_color_wheel_popup_uninstall(ui::ui_context& ui, void* user_data);
		static void on_color_wheel_edit_begin(void* user_data);
		static void on_color_wheel_data_changed(void* user_data);

	private:
		editor_color_field_config_t _config			   = {};
		vector_t<color_t*>			_fields			   = {};
		vector_t<f32*>				_float4_fields	   = {};
		ui::ui_context*				_ui				   = nullptr;
		editor_popup_color_wheel_t* _color_wheel_popup = nullptr;
		color_t						_color			   = {};
		ui::widget_id_t				_root			   = NULL_WIDGET;
		ui::widget_id_t				_swatch			   = NULL_WIDGET;
		ui::widget_id_t				_label			   = NULL_WIDGET;
		bool						_mixed			   = false;
		bool						_edit_active	   = false;
		bool						_edit_dirty		   = false;
	};
}
