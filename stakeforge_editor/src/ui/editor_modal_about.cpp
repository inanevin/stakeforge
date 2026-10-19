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

#include "editor_modal_about.hpp"
#include "editor_modal_controller.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_modal_about_t::open(ui::ui_context& ui, const char* version, const char* build)
	{
		close();

		_version = version;
		_build	 = build;

		const editor_modal_button_desc_t  button{.text = "Close"};
		const editor_modal_content_desc_t content{
			.init	   = init_content,
			.uninit	   = uninit_content,
			.user_data = this,
		};

		editor_modal_controller_t::find(ui)->request_modal("About Stakeforge", "made with love in Helsinki <3", true, &button, 1, &content);
	}

	void editor_modal_about_t::close()
	{
		if (_ui != nullptr)
			editor_modal_controller_t::find(*_ui)->close_modal();
	}

	void editor_modal_about_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		editor_modal_about_t& about = *static_cast<editor_modal_about_t*>(user_data);
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		about._ui	= &ui;
		about._root = ui.allocate_widget();
		ui.set_widget_debug_name(about._root, "about_content");
		tree.attach(parent, about._root);

		ui::layout_in_t& root_in = tree.in(about._root);
		root_in.size_mode_x		 = ui::axis_mode_e::max_children;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing;

		const char* lines[] = {
			about._version,
			about._build,
			"Copyright (c) 2025-present Inan Evin.",
			"GNU General Public License, version 3 (GPLv3).",
			"With the Stakeforge Game Linking Exception, version 1.0.",
			"Provided without warranty.",
		};

		for (const char* line : lines)
		{
			const ui::widget_id_t label = ui.allocate_widget();
			tree.attach(about._root, label);
			ui.set_widget_text(label, line);
			ui.get_paint().set_text(
				label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
	}

	void editor_modal_about_t::uninit_content(void* user_data)
	{
		editor_modal_about_t& about = *static_cast<editor_modal_about_t*>(user_data);

		about._ui->deallocate_widget(about._root);
		about._root	   = NULL_WIDGET;
		about._ui	   = nullptr;
		about._version = nullptr;
		about._build   = nullptr;
	}
}
