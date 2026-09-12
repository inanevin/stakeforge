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
#include "ui/widgets/outliner/editor_widget_outliner.hpp"

namespace sfg
{
	class editor_panel_entities_t final : public editor_panel_t
	{
	public:
		editor_panel_entities_t();
		~editor_panel_entities_t() override								   = default;
		editor_panel_entities_t(const editor_panel_entities_t&)			   = delete;
		editor_panel_entities_t& operator=(const editor_panel_entities_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_entities();
		void refresh_entity_name(entity_id_t entity);
		void set_edit_world(editor_world_handle_t world);
		void show_entity(entity_guid_t guid);

	private:
		editor_widget_outliner_t _outliner;
	};
}
