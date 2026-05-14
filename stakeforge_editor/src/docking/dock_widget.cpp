// Copyright (c) 2025 Inan Evin

#include "docking/dock_widget.hpp"
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
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::none;

		_root_node = create_leaf_node(_root);
	}

	void dock_widget_t::uninit()
	{
		dock_node_t& root_node = _dock_nodes.get(_root_node);
		root_node.panels.clear();
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
		tree.draw_order(node.widget) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& widget_in = tree.in(node.widget);
		widget_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		widget_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		widget_in.size_value	   = {1.0f, 1.0f};
		widget_in.flow			   = ui::flow_e::column;
		widget_in.child_spacing	   = 0.0f;
		widget_in.child_margins	   = {0.0f, 0.0f, 0.0f, 0.0f};

		node.tab_area = ui.allocate_widget();
		ui.set_widget_debug_name(node.tab_area, "dock_node_tab_area");
		tree.attach(node.widget, node.tab_area);
		tree.draw_order(node.tab_area) = tree.draw_order_const(node.widget) + 1;

		ui::layout_in_t& tab_in = tree.in(node.tab_area);
		tab_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		tab_in.size_mode_y		= ui::axis_mode_e::fixed;
		tab_in.size_value		= {1.0f, theme.item_height};
		tab_in.flow				= ui::flow_e::row;
		tab_in.child_spacing	= 0.0f;
		tab_in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		ui::vg_rect_paint_t tab_rect = {};
		tab_rect.fill_color_a		 = theme.color_bg0;
		tab_rect.fill_color_b		 = theme.color_bg0;
		paint.set_rect(node.tab_area, tab_rect);

		node.body = ui.allocate_widget();
		ui.set_widget_debug_name(node.body, "dock_node_body");
		tree.attach(node.widget, node.body);
		tree.draw_order(node.body) = tree.draw_order_const(node.widget) + 1;

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
