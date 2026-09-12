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

#include "ui/widgets/popups/editor_popup_world_view_settings.hpp"
#include "ui/panels/editor_theme.hpp"
#include "world/editor_world_view_settings.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_popup_world_view_settings_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_popup_world_view_settings_config_t& config)
	{
		SFG_ASSERT(config.settings != nullptr);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "world_view_settings_popup");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = theme.item_width * 3.0f;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		void* settings = config.settings;
		_reflection.init(ui,
						 _root,
						 {
							 .objects = {.data = &settings, .size = 1},
							 .type_id = type_id_t<editor_world_view_settings_t>::value,
						 });
	}

	void editor_popup_world_view_settings_t::uninit()
	{
		_reflection.uninit();
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}
}
