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
