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
	class editor_modal_about_t final
	{
	public:
		editor_modal_about_t()											 = default;
		~editor_modal_about_t()											 = default;
		editor_modal_about_t(const editor_modal_about_t&)				 = delete;
		editor_modal_about_t& operator=(const editor_modal_about_t&)	 = delete;
		editor_modal_about_t(editor_modal_about_t&&) noexcept			 = default;
		editor_modal_about_t& operator=(editor_modal_about_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void open(ui::ui_context& ui, const char* version, const char* build);
		void close();

	private:
		static void init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void uninit_content(void* user_data);

	private:
		ui::ui_context* _ui		 = nullptr;
		const char*		_version = nullptr;
		const char*		_build	 = nullptr;
		ui::widget_id_t _root	 = NULL_WIDGET;
	};
}
