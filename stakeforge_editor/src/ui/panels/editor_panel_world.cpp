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
#include "ui/panels/editor_panel_world.hpp"
#include "editor_world_controller.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>

namespace sfg
{
	editor_panel_world_t::editor_panel_world_t()
	{
		set_type(editor_panel_type_e::world);
		refresh_title();
		set_icon(ICON_GLOBE);
	}

	void editor_panel_world_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_world_view.init(ui, _root);

		editor_world_controller_t& worlds = editor_world_controller_t::get();
		_world_view.set_edit_world(worlds.get_main_world_handle());
		set_panel_name(worlds.get_main_world_handle().is_null() ? "" : worlds.get_main_world_name());
		set_world_dirty(worlds.is_main_world_dirty());
	}

	void editor_panel_world_t::uninit()
	{
		_world_view.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_world_t::set_edit_world(editor_world_handle_t world)
	{
		_world_view.set_edit_world(world);
	}

	void editor_panel_world_t::set_panel_name(const char* name)
	{
		_panel_name = name;
		refresh_title(_panel_name.c_str(), nullptr, _world_dirty);
	}

	void editor_panel_world_t::set_world_dirty(bool dirty)
	{
		if (_world_dirty == dirty)
			return;

		_world_dirty = dirty;
		refresh_title(_panel_name.c_str(), nullptr, _world_dirty);
	}
}
