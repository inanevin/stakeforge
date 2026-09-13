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
	struct editor_widget_fold_config_t
	{
		ui::vg_rect_paint_t background						  = {};
		const char*			label							  = nullptr;
		void (*on_pressed)(void* user_data)					  = nullptr;
		void (*on_fold_changed)(bool folded, void* user_data) = nullptr;
		void* user_data										  = nullptr;
		bool  folded										  = false;
		bool  settings_button								  = false;
		bool  background_frame								  = false;
		bool  header_frame									  = true;
	};

	class editor_widget_fold_t final
	{
	public:
		editor_widget_fold_t()										 = default;
		~editor_widget_fold_t()										 = default;
		editor_widget_fold_t(const editor_widget_fold_t&)			 = delete;
		editor_widget_fold_t& operator=(const editor_widget_fold_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_fold_config_t& config);
		void uninit();
		void set_fold(bool folded);
		void set_text(const char* text);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline ui::widget_id_t get_body() const
		{
			return _body;
		}

		inline ui::widget_id_t get_settings_button() const
		{
			return _settings_button;
		}

		inline bool is_folded() const
		{
			return _folded;
		}

	private:
		void refresh();

		static void on_header_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_background_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		ui::ui_context* _ui									   = nullptr;
		void (*_on_pressed)(void* user_data)				   = nullptr;
		void (*_on_fold_changed)(bool folded, void* user_data) = nullptr;
		void*			_user_data							   = nullptr;
		ui::widget_id_t _root								   = NULL_WIDGET;
		ui::widget_id_t _header								   = NULL_WIDGET;
		ui::widget_id_t _icon								   = NULL_WIDGET;
		ui::widget_id_t _label								   = NULL_WIDGET;
		ui::widget_id_t _body								   = NULL_WIDGET;
		ui::widget_id_t _settings_button					   = NULL_WIDGET;
		bool			_folded								   = false;
	};
}
