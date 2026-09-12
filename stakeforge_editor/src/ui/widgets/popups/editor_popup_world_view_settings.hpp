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

#include "ui/widgets/editor_widget_reflection.hpp"

namespace sfg
{
	struct editor_world_view_settings_t;

	struct editor_popup_world_view_settings_config_t
	{
		editor_world_view_settings_t* settings = nullptr;
	};

	class editor_popup_world_view_settings_t final
	{
	public:
		editor_popup_world_view_settings_t()													 = default;
		~editor_popup_world_view_settings_t()													 = default;
		editor_popup_world_view_settings_t(const editor_popup_world_view_settings_t&)			 = delete;
		editor_popup_world_view_settings_t& operator=(const editor_popup_world_view_settings_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_popup_world_view_settings_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline bool is_initialized() const
		{
			return _ui != nullptr;
		}

	private:
		editor_widget_reflection_t _reflection = {};
		ui::ui_context*			   _ui		   = nullptr;
		ui::widget_id_t			   _root	   = NULL_WIDGET;
	};
}
