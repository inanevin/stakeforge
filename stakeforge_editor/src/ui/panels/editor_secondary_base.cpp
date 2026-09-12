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
#include "ui/panels/editor_secondary_base.hpp"
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_secondary_base_t::init(ui::ui_context& ui, ui::widget_id_t parent, window_runtime_t& runtime)
	{
		_ui						= &ui;
		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "secondary_base");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		dock_widget_config_t dock_config   = {};
		dock_config.runtime				   = &runtime;
		dock_config.root_drag_out_behavior = dock_widget_root_drag_out_e::close_window;
		_dock_widget.init(ui, _root, dock_config);
		ui::layout_in_t& dock_in = tree.in(_dock_widget.get_root());
		dock_in.size_mode_y		 = ui::axis_mode_e::fill;
	}

	void editor_secondary_base_t::uninit()
	{
		_dock_widget.uninit();

		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

}
