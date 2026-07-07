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

#include "ui/docking/dock_widget.hpp"
#include "ui/editor_main_toolbar.hpp"
#include "ui/widgets/editor_widgets_file_menu.hpp"
#include <sfg/math/vec2i16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct window_runtime_t;

	class editor_primary_base_t final
	{
	public:
		editor_primary_base_t()											   = default;
		~editor_primary_base_t()										   = default;
		editor_primary_base_t(const editor_primary_base_t&)				   = delete;
		editor_primary_base_t& operator=(const editor_primary_base_t&)	   = delete;
		editor_primary_base_t(editor_primary_base_t&&) noexcept			   = default;
		editor_primary_base_t& operator=(editor_primary_base_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, window_runtime_t& runtime);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void prompt_no_project_modal();
		void set_current_project_name(const char* name);
		bool is_window_drag_region(const vec2i16_t& pos) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::ui_context& get_ui()
		{
			return *_ui;
		}
		inline const ui::ui_context& get_ui() const
		{
			return *_ui;
		}
		inline dock_widget_t& get_dock_widget()
		{
			return _dock_widget;
		}
		inline const dock_widget_t& get_dock_widget() const
		{
			return _dock_widget;
		}
		inline editor_main_toolbar_t& get_main_toolbar()
		{
			return _main_toolbar;
		}
		inline const editor_main_toolbar_t& get_main_toolbar() const
		{
			return _main_toolbar;
		}

	private:
		ui::ui_context*		  _ui			   = nullptr;
		ui::widget_id_t		  _base			   = NULL_WIDGET;
		ui::widget_id_t		  _project_label   = NULL_WIDGET;
		ui::widget_id_t		  _top_row_left	   = NULL_WIDGET;
		ui::widget_id_t		  _top_row_strikes = NULL_WIDGET;
		ui::widget_id_t		  _top_mid_file	   = NULL_WIDGET;
		ui::widget_id_t		  _top_mid_util	   = NULL_WIDGET;
		ui::widget_id_t		  _label_wrap	   = NULL_WIDGET;
		dock_widget_t		  _dock_widget;
		editor_file_menu_t	  _file_menu;
		editor_main_toolbar_t _main_toolbar;

		static void on_no_project_open(void* user_data);
		static void on_no_project_create(void* user_data);
	};
}
