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
	class ui_context;
}

namespace sfg
{
	struct editor_widget_progress_bar_config_t
	{
		const char* progress_text	= "";
		f32			progress_amount = 0.0f;
		f32			frame_height	= 0.0f;
	};

	class editor_widget_progress_bar_t final
	{
	public:
		editor_widget_progress_bar_t()													 = default;
		~editor_widget_progress_bar_t()													 = default;
		editor_widget_progress_bar_t(const editor_widget_progress_bar_t&)				 = delete;
		editor_widget_progress_bar_t& operator=(const editor_widget_progress_bar_t&)	 = delete;
		editor_widget_progress_bar_t(editor_widget_progress_bar_t&&) noexcept			 = default;
		editor_widget_progress_bar_t& operator=(editor_widget_progress_bar_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_progress_bar_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_progress(f32 progress);
		void update_progress_text(const char* text);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_progress_amount();

	private:
		ui::ui_context* _ui					   = nullptr;
		ui::widget_id_t _root				   = NULL_WIDGET;
		ui::widget_id_t _progress_text		   = NULL_WIDGET;
		ui::widget_id_t _progress_frame		   = NULL_WIDGET;
		ui::widget_id_t _progress_fill		   = NULL_WIDGET;
		ui::widget_id_t _progress_amount_label = NULL_WIDGET;
		f32				_progress_amount	   = 0.0f;
	};
}
