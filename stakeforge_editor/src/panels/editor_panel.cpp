// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_panel_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui						= &ui;
		ui::layout_tree_t& tree = ui.get_tree();

		_root = tree.allocate();
		ui.set_widget_debug_name(_root, "editor_panel");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::flow;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
	}

	void editor_panel_t::uninit()
	{
		if (_ui != nullptr)
			_ui->clear_widget_debug_name(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}
}
