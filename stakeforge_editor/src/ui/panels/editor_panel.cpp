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
#include "ui/panels/editor_panel.hpp"
#include "editor_surface_controller.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	sid_t g_next_editor_panel_instance_id = 1;

	editor_panel_t::editor_panel_t()
	{
		_instance_id = g_next_editor_panel_instance_id++;
	}

	void editor_panel_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui						= &ui;
		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_panel");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.pos_mode_x		 = ui::pos_mode_e::flow;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
	}

	void editor_panel_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_panel_t::serialize(nlohmann::json& j) const
	{
	}

	void editor_panel_t::deserialize(const nlohmann::json&)
	{
	}

	void editor_panel_t::assign(ui::ui_context& ui, ui::widget_id_t parent)
	{
		if (!is_inited())
		{
			init(ui, parent);
			return;
		}

		if (_ui != &ui)
		{
			uninit();
			init(ui, parent);
			return;
		}

		_ui->get_tree().attach(parent, _root);
	}

	void editor_panel_t::deassign()
	{
		_ui->get_tree().detach(_root);
	}

	void editor_panel_t::make_visible(bool visible)
	{
		_ui->get_tree().set_visible(_root, visible, false);
	}

	void editor_panel_t::set_title(const char* title)
	{
		_title = title;
	}

	void editor_panel_t::refresh_title(const char* detail, const char* detail_prefix, bool dirty)
	{
		_title_text = editor_panel_type_to_string(_type);

		if (detail != nullptr && detail[0] != '\0')
		{
			if (detail_prefix != nullptr)
				_title_text = detail_prefix;
			else
				_title_text += ": ";
			_title_text += detail;
		}

		if (dirty)
			_title_text += "*";

		set_title(_title_text.c_str());
		if (_ui != nullptr)
			editor_surface_controller_t::get().refresh_panel_title(this);
	}

	void editor_panel_t::set_icon(const char* icon)
	{
		_icon = icon;
	}

	void editor_panel_t::set_type(editor_panel_type_e type)
	{
		_type = type;
	}

	void editor_panel_t::set_instance_id(sid_t instance_id)
	{
		SFG_ASSERT(instance_id != 0);
		_instance_id = instance_id;
		if (g_next_editor_panel_instance_id <= instance_id)
			g_next_editor_panel_instance_id = instance_id + 1;
	}

	void editor_panel_t::set_sub_item_id(sid_t sub_item_id)
	{
		_sub_item_id = sub_item_id;
	}
}
