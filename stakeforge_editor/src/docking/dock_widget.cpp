// Copyright (c) 2025 Inan Evin

#include "docking/dock_widget.hpp"
#include "panels/editor_panel.hpp"
#include "panels/editor_panel_factory.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void dock_widget_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui						= &ui;
		ui::layout_tree_t& tree = ui.get_tree();

		_dock_nodes.reserve(DOCK_POOL_INITIAL_CAPACITY);
		_dock_borders.reserve(DOCK_POOL_INITIAL_CAPACITY);

		{
			_root_node			   = alloc_dock_node();
			dock_node_t& root_node = _dock_nodes.get(_root_node);
			root_node.node_type	   = dock_node_type_e::leaf;
			root_node.panels.push_back(editor_panel_factory_t::create_panel(editor_panel_type_e::entities));
			root_node.panels.push_back(editor_panel_factory_t::create_panel(editor_panel_type_e::world));
		}

		_root = tree.allocate();
		ui.set_widget_debug_name(_root, "dock_widget");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::none;
	}

	void dock_widget_t::uninit()
	{
		if (_ui != nullptr)
		{
			dock_node_t& root_node = _dock_nodes.get(_root_node);
			for (editor_panel_t* panel : root_node.panels)
			{
				editor_panel_factory_t::delete_panel(panel);
			}
			root_node.panels.clear();
			free_dock_node(_root_node);
			_ui->clear_widget_debug_name(_root);
		}

		_ui		   = nullptr;
		_root	   = NULL_WIDGET;
		_root_node = {};
		_dock_nodes.clear();
		_dock_borders.clear();
	}

	dock_node_handle_t dock_widget_t::alloc_dock_node()
	{
		return _dock_nodes.emplace();
	}

	void dock_widget_t::free_dock_node(dock_node_handle_t handle)
	{
		_dock_nodes.remove(handle);
	}

	dock_border_handle_t dock_widget_t::alloc_dock_border()
	{
		return _dock_borders.emplace();
	}

	void dock_widget_t::free_dock_border(dock_border_handle_t handle)
	{
		_dock_borders.remove(handle);
	}
}
