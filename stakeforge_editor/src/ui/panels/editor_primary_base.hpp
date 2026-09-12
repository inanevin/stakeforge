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

		void init(ui::ui_context& ui, ui::widget_id_t parent, window_runtime_t& runtime);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

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
	};
}
