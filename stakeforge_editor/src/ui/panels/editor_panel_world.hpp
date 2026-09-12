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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include <sfg/data/string.hpp>

namespace sfg
{
	class editor_panel_world_t final : public editor_panel_t
	{
	public:
		editor_panel_world_t();
		~editor_panel_world_t() override							 = default;
		editor_panel_world_t(const editor_panel_world_t&)			 = delete;
		editor_panel_world_t& operator=(const editor_panel_world_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void set_edit_world(editor_world_handle_t world);
		void set_panel_name(const char* name);
		void set_world_dirty(bool dirty);

		inline editor_widget_world_view_t& get_world_view()
		{
			return _world_view;
		}

	private:
		editor_widget_world_view_t _world_view;
		string_t				   _panel_name	= {};
		bool					   _world_dirty = false;
	};
}
