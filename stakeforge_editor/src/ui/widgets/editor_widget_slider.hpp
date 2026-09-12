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

#include <sfg/data/span.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_slider_field_t
	{
		span_t<f32*> fields = {};
	};

	struct editor_slider_config_t
	{
		editor_slider_field_t field			= {};
		f32					  min_value		= 0.0f;
		f32					  max_value		= 1.0f;
		f32					  width			= 0.0f;
		u8					  decimal_count = 2;
		bool				  fixed_width	= false;
		bool				  display_label = true;
	};

	class editor_slider_t final
	{
	public:
		editor_slider_t()								   = default;
		~editor_slider_t()								   = default;
		editor_slider_t(const editor_slider_t&)			   = delete;
		editor_slider_t& operator=(const editor_slider_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_slider_config_t& config);
		void uninit();
		void set_value(f32 value);
		void update_field_data(editor_slider_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline f32 get_value() const
		{
			return _value;
		}

	private:
		void refresh();
		void refresh_label();
		void set_value_from_pos(const vec2f_t& pos);
		void modify_field();

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_slider_config_t _config		   = {};
		char				   _label_text[32] = {};
		ui::ui_context*		   _ui			   = nullptr;
		ui::widget_id_t		   _root		   = NULL_WIDGET;
		ui::widget_id_t		   _slider		   = NULL_WIDGET;
		ui::widget_id_t		   _bg			   = NULL_WIDGET;
		ui::widget_id_t		   _icon		   = NULL_WIDGET;
		ui::widget_id_t		   _label		   = NULL_WIDGET;
		f32					   _value		   = 0.0f;
		bool				   _mixed		   = false;
	};
}
