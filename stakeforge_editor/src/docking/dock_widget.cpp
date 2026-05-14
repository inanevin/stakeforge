// Copyright (c) 2025 Inan Evin

#include "docking/dock_widget.hpp"
#include "panels/editor_panel.hpp"
#include "panels/editor_panel_factory.hpp"
#include "panels/editor_panel_types.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
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

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "dock_widget");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::none;

		_root_node = create_leaf_node(_root);
		dock_node_add_panel(_dock_nodes.get(_root_node), editor_panel_factory_t::create_panel(editor_panel_type_e::entities));
	}

	void dock_widget_t::uninit()
	{
		dock_node_t& root_node = _dock_nodes.get(_root_node);
		for (editor_panel_t* panel : root_node.panels)
		{
			panel->uninit();
			editor_panel_factory_t::delete_panel(panel);
		}
		root_node.panels.clear();
		root_node.tab_area.uninit();
		free_dock_node(_root_node);
		_ui->deallocate_widget(_root);

		_ui		   = nullptr;
		_root	   = NULL_WIDGET;
		_root_node = {};
		_dock_nodes.clear();
		_dock_borders.clear();
	}

	dock_node_handle_t dock_widget_t::create_leaf_node(ui::widget_id_t parent)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const dock_node_handle_t handle = alloc_dock_node();
		dock_node_t&			 node	= _dock_nodes.get(handle);
		node.node_type					= dock_node_type_e::leaf;

		node.widget = ui.allocate_widget();
		ui.set_widget_debug_name(node.widget, "dock_node_leaf");
		tree.attach(parent, node.widget);

		ui::layout_in_t& widget_in = tree.in(node.widget);
		widget_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		widget_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		widget_in.size_value	   = {1.0f, 1.0f};
		widget_in.flow			   = ui::flow_e::column;
		widget_in.child_spacing	   = 0.0f;
		widget_in.child_margins	   = {0.0f, 0.0f, 0.0f, 0.0f};

		node.tab_area.init(ui, node.widget);

		node.body = ui.allocate_widget();
		ui.set_widget_debug_name(node.body, "dock_node_body");
		tree.attach(node.widget, node.body);

		ui::layout_in_t& body_in = tree.in(node.body);
		body_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		body_in.size_mode_y		 = ui::axis_mode_e::fill;
		body_in.size_value		 = {1.0f, 1.0f};
		body_in.flow			 = ui::flow_e::row;
		body_in.child_spacing	 = 0.0f;
		body_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		ui::vg_rect_paint_t body_rect = {};
		body_rect.fill_color_a		  = theme.color_bg3;
		body_rect.fill_color_b		  = theme.color_bg3;
		paint.set_rect(node.body, body_rect);

		return handle;
	}

	void dock_widget_t::dock_node_add_panel(dock_node_t& node, editor_panel_t* panel)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);
		SFG_ASSERT(panel != nullptr);

		node.tab_area.add_tab(panel->get_title());
		panel->assign(*_ui, node.body);
		node.panels.push_back(panel);
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
