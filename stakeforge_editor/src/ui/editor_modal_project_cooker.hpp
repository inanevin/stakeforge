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

#include "editor_project_cook_options.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"

namespace sfg
{
	class editor_modal_project_cooker_t final
	{
	public:
		editor_modal_project_cooker_t()												   = default;
		~editor_modal_project_cooker_t()											   = default;
		editor_modal_project_cooker_t(const editor_modal_project_cooker_t&)			   = delete;
		editor_modal_project_cooker_t& operator=(const editor_modal_project_cooker_t&) = delete;

		void request(editor_modal_controller_t& modal, const editor_project_cook_options_t& options);

	private:
		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		static void init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void uninit_content(void* user_data);
		static void on_cancel(void* user_data);
		static void on_cook(void* user_data);

	private:
		editor_project_cook_options_t _options	  = {};
		editor_widget_reflection_t	  _reflection = {};
	};
}
