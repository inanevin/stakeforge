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

#include "ui/widgets/editor_widgets_common.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	struct key_event_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_checkbox_field_t
	{
		span_t<u8*> fields	   = {};
		size_t		field_size = sizeof(u8);
	};

	struct editor_checkbox_config_t
	{
		editor_checkbox_field_t	  field		= {};
		editor_widget_callbacks_t callbacks = {};
	};

	class editor_checkbox_t final
	{
	public:
		editor_checkbox_t()									   = default;
		~editor_checkbox_t()								   = default;
		editor_checkbox_t(const editor_checkbox_t&)			   = delete;
		editor_checkbox_t& operator=(const editor_checkbox_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_checkbox_config_t& config);
		void uninit();
		void update_field_data(editor_checkbox_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_checked() const
		{
			return _checked;
		}

	private:
		void refresh();
		void toggle();
		void modify_field();

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);

	private:
		editor_checkbox_config_t _config  = {};
		vector_t<u8*>			 _fields  = {};
		ui::ui_context*			 _ui	  = nullptr;
		ui::widget_id_t			 _root	  = NULL_WIDGET;
		ui::widget_id_t			 _check	  = NULL_WIDGET;
		bool					 _checked = false;
		bool					 _mixed	  = false;
	};
}
