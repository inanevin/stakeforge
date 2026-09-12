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
	enum class editor_widget_fold_label_button_style_e : u8
	{
		none,
		container_buttons,
		container_item_buttons,
	};

	struct editor_widget_fold_label_config_t
	{
		const char*								label		 = nullptr;
		f32										indentation	 = 0.0f;
		editor_widget_fold_label_button_style_e button_style = editor_widget_fold_label_button_style_e::none;
		bool									folded		 = false;
		bool									sub_item	 = false;
	};

	class editor_widget_fold_label_t final
	{
	public:
		editor_widget_fold_label_t()											 = default;
		~editor_widget_fold_label_t()											 = default;
		editor_widget_fold_label_t(const editor_widget_fold_label_t&)			 = delete;
		editor_widget_fold_label_t& operator=(const editor_widget_fold_label_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_fold_label_config_t& config);
		void uninit();
		void set_fold(bool folded);
		void clear_children();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline ui::widget_id_t get_body() const
		{
			return _body;
		}

		inline ui::widget_id_t get_add_button() const
		{
			return _add_button;
		}

		inline ui::widget_id_t get_reset_button() const
		{
			return _reset_button;
		}

		inline ui::widget_id_t get_remove_button() const
		{
			return _remove_button;
		}

		inline bool is_folded() const
		{
			return _folded;
		}

	private:
		void refresh();

		static void on_header_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		ui::ui_context* _ui			   = nullptr;
		ui::widget_id_t _root		   = NULL_WIDGET;
		ui::widget_id_t _header		   = NULL_WIDGET;
		ui::widget_id_t _icon		   = NULL_WIDGET;
		ui::widget_id_t _body		   = NULL_WIDGET;
		ui::widget_id_t _add_button	   = NULL_WIDGET;
		ui::widget_id_t _reset_button  = NULL_WIDGET;
		ui::widget_id_t _remove_button = NULL_WIDGET;
		bool			_folded		   = false;
	};
}
