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
#include "ui/panels/editor_panel_entities.hpp"
#include "editor_world_controller.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

namespace sfg
{
	editor_panel_entities_t::editor_panel_entities_t()
	{
		set_type(editor_panel_type_e::entities);
		set_title(editor_panel_type_to_string(editor_panel_type_e::entities));
		set_icon(ICON_CUBE);
	}

	void editor_panel_entities_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_outliner.init(ui, _root);
		_outliner.set_edit_world(editor_world_controller_t::get().get_main_world_handle());
		_outliner.refresh_entities();
	}

	void editor_panel_entities_t::uninit()
	{
		_outliner.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_entities_t::refresh_entities()
	{
		_outliner.refresh_entities();
	}

	void editor_panel_entities_t::refresh_entity_name(entity_id_t entity)
	{
		_outliner.refresh_entity_name(entity);
	}

	void editor_panel_entities_t::set_edit_world(editor_world_handle_t world)
	{
		_outliner.set_edit_world(world);
	}

	void editor_panel_entities_t::show_entity(entity_guid_t guid)
	{
		_outliner.show_entity(guid);
	}
}
