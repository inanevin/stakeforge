// Copyright (c) 2025 Inan Evin

#include "docking/dock_widget.hpp"
#include "editor_app.hpp"
#include "editor_payload_type.hpp"
#include "panels/editor_panel.hpp"
#include "panels/editor_panel_factory.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2u16.hpp>
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
	}

	void dock_widget_t::uninit()
	{
		if (!_root_node.is_null())
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
		}

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

		editor_tab_area_config_t tab_config = {};
		tab_config.tab_switched				= on_leaf_tab_switched;
		tab_config.tab_dragged_out			= on_leaf_tab_dragged_out;
		tab_config.user_data				= this;
		tab_config.can_drag_out				= true;
		node.tab_area.init(ui, node.widget, tab_config);

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
		body_rect.fill_color_a		  = theme.color_panel;
		body_rect.fill_color_b		  = theme.color_panel;
		paint.set_rect(node.body, body_rect);

		return handle;
	}

	void dock_widget_t::set_root_node(dock_node_handle_t handle)
	{
		SFG_ASSERT(_dock_nodes.is_valid(handle));
		_root_node = handle;
	}

	void dock_widget_t::dock_node_add_panel(dock_node_handle_t handle, editor_panel_t* panel)
	{
		dock_node_add_panel(_dock_nodes.get(handle), panel);
	}

	void dock_widget_t::dock_node_add_panel(dock_node_t& node, editor_panel_t* panel)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);
		SFG_ASSERT(panel != nullptr);

		node.tab_area.add_tab(panel->get_title());
		panel->assign(*_ui, node.body);
		node.panels.push_back(panel);
		node.tab_area.select_tab(TO_SID(panel->get_title()));
	}

	void dock_widget_t::dock_node_remove_panel(dock_node_t& node, sid_t identifier)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);

		for (auto it = node.panels.begin(); it != node.panels.end(); ++it)
		{
			editor_panel_t* panel = *it;
			if (TO_SID(panel->get_title()) == identifier)
			{
				const ui::layout_out_t& out	 = _ui->get_tree().out(node.widget);
				const vec2u16_t			size = {static_cast<u16>(out.size.x), static_cast<u16>(out.size.y)};
				panel->deassign();
				node.panels.erase(it);
				editor_app_t::get().create_payload(panel->get_title(), editor_payload_type_e::panel, panel, size);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void dock_widget_t::set_leaf_active_panel(dock_node_t& node, sid_t active_tab)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);

		for (editor_panel_t* panel : node.panels)
			panel->make_visible(TO_SID(panel->get_title()) == active_tab);
	}

	void dock_widget_t::on_leaf_tab_switched(editor_tab_area_t& tab_area, sid_t identifier, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (dock_node_t& node : dock_widget._dock_nodes)
		{
			if (&node.tab_area == &tab_area)
			{
				dock_widget.set_leaf_active_panel(node, identifier);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void dock_widget_t::on_leaf_tab_dragged_out(editor_tab_area_t& tab_area, sid_t identifier, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (dock_node_t& node : dock_widget._dock_nodes)
		{
			if (&node.tab_area == &tab_area)
			{
				dock_widget.dock_node_remove_panel(node, identifier);
				return;
			}
		}

		SFG_ASSERT(false);
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
