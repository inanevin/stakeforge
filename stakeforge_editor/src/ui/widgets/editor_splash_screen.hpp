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

#include "ui/widgets/editor_widget_progress_bar.hpp"

#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_splash_screen_config_t
	{
		vec2u16_t owner_size = {};
	};

	class editor_splash_screen_t final
	{
	public:
		editor_splash_screen_t()											 = default;
		~editor_splash_screen_t()											 = default;
		editor_splash_screen_t(const editor_splash_screen_t&)				 = delete;
		editor_splash_screen_t& operator=(const editor_splash_screen_t&)	 = delete;
		editor_splash_screen_t(editor_splash_screen_t&&) noexcept			 = default;
		editor_splash_screen_t& operator=(editor_splash_screen_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_splash_screen_config_t& config);
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
		ui::ui_context*				 _ui		   = nullptr;
		ui::widget_id_t				 _root		   = NULL_WIDGET;
		ui::widget_id_t				 _texture_bg   = NULL_WIDGET;
		ui::widget_id_t				 _strikes	   = NULL_WIDGET;
		ui::widget_id_t				 _column	   = NULL_WIDGET;
		ui::widget_id_t				 _title		   = NULL_WIDGET;
		ui::widget_id_t				 _version	   = NULL_WIDGET;
		ui::widget_id_t				 _build		   = NULL_WIDGET;
		ui::widget_id_t				 _project_path = NULL_WIDGET;
		ui::widget_id_t				 _project_name = NULL_WIDGET;
		editor_widget_progress_bar_t _progress	   = {};
	};
}
