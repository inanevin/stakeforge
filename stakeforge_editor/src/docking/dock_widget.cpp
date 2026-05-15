// Copyright (c) 2025 Inan Evin

#include "docking/dock_widget.hpp"
#include "editor_app.hpp"
#include "editor_payload_controller.hpp"
#include "editor_payload_type.hpp"
#include "panels/editor_panel.hpp"
#include "panels/editor_panel_factory.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	namespace
	{
		bool contains(const vec4f_t& rect, const vec2f_t& p)
		{
			return p.x >= rect.x && p.x <= rect.x + rect.z && p.y >= rect.y && p.y <= rect.y + rect.w;
		}

		vec4f_t expand_rect(const vec4f_t& rect, f32 value)
		{
			return {rect.x - value, rect.y - value, rect.z + value * 2.0f, rect.w + value * 2.0f};
		}
	}

	void dock_widget_t::init(ui::ui_context& ui, ui::widget_id_t parent, const dock_widget_config_t& config)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui						= &ui;
		_runtime				= config.runtime;
		_config					= config;
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

		editor_app_t::get().get_payload_controller().register_listener(on_payload_drop, on_payload_tick, on_payload_end, this);
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
		editor_app_t::get().get_payload_controller().unregister_listener(this);

		_ui					  = nullptr;
		_runtime			  = nullptr;
		_root				  = NULL_WIDGET;
		_root_node			  = {};
		_config				  = {};
		_panel_payload_active = false;
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

		paint.set_custom(node.widget, draw_leaf_dock_previews, this);

		editor_tab_area_config_t tab_config = {};
		tab_config.tab_switched				= on_leaf_tab_switched;
		tab_config.tab_dragged_out			= on_leaf_tab_dragged_out;
		tab_config.drag_out_allowed			= is_leaf_tab_drag_out_allowed;
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
				panel->uninit();
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

	void dock_widget_t::update_dock_previews(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos)
	{
		_panel_payload_active = payload.type == editor_payload_type_e::panel;
		clear_dock_previews();

		if (!_panel_payload_active)
			return;

		SFG_ASSERT(_runtime != nullptr);

		const vec2f_t mouse = {
			static_cast<f32>(abs_mouse_pos.x - _runtime->pos.x),
			static_cast<f32>(abs_mouse_pos.y - _runtime->pos.y),
		};

		dock_node_t* node = find_leaf_at(mouse);
		if (node == nullptr)
			return;

		update_leaf_dock_previews(*node, mouse);
	}

	void dock_widget_t::clear_dock_previews()
	{
		for (dock_node_t& node : _dock_nodes)
		{
			if (node.node_type == dock_node_type_e::leaf)
			{
				node.is_payload_over = false;
				node.hovered_preview = dock_preview_e::none;
			}
		}
	}

	void dock_widget_t::update_leaf_dock_previews(dock_node_t& node, const vec2f_t& mouse)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);

		node.is_payload_over = true;
		node.hovered_preview = dock_preview_e::none;

		const ui::layout_out_t& out		  = _ui->get_tree().out(node.widget);
		const editor_theme_t&	theme	  = editor_theme_t::get();
		const f32				margin	  = theme.item_height;
		const f32				x		  = out.pos.x + margin;
		const f32				y		  = out.pos.y + margin;
		const f32				w		  = math::max(theme.item_height * 4.0f, out.size.x - margin * 2.0f);
		const f32				h		  = math::max(theme.item_height * 4.0f, out.size.y - margin * 2.0f);
		const f32				preview_w = theme.item_height * 2.75f;
		const f32				preview_h = theme.item_height * 2.75f;
		const f32				gap		  = theme.item_spacing * 1.5f;
		const f32				center_x  = x + w * 0.5f;
		const f32				center_y  = y + h * 0.5f;
		const f32				base_x	  = center_x - preview_w * 0.5f;
		const f32				base_y	  = center_y - preview_h * 0.5f;

		node.preview_rects[static_cast<u32>(dock_preview_e::top)]	 = {base_x, base_y - preview_h - gap, preview_w, preview_h};
		node.preview_rects[static_cast<u32>(dock_preview_e::left)]	 = {base_x - preview_w - gap, base_y, preview_w, preview_h};
		node.preview_rects[static_cast<u32>(dock_preview_e::bottom)] = {base_x, base_y + preview_h + gap, preview_w, preview_h};
		node.preview_rects[static_cast<u32>(dock_preview_e::right)]	 = {base_x + preview_w + gap, base_y, preview_w, preview_h};
		node.preview_rects[static_cast<u32>(dock_preview_e::center)] = {base_x, base_y, preview_w, preview_h};

		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			if (contains(node.preview_rects[i], mouse))
			{
				node.hovered_preview = static_cast<dock_preview_e>(i);
				return;
			}
		}
	}

	void dock_widget_t::collapse_empty_leaf_after_drag_out(dock_node_t&)
	{
	}

	dock_node_t* dock_widget_t::find_leaf_at(const vec2f_t& mouse)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (dock_node_t& node : _dock_nodes)
		{
			if (node.node_type == dock_node_type_e::leaf)
			{
				const ui::layout_out_t& out	 = tree.out(node.widget);
				const vec4f_t			rect = {out.pos.x, out.pos.y, out.size.x, out.size.y};
				if (contains(rect, mouse))
					return &node;
			}
		}
		return nullptr;
	}

	const dock_node_t* dock_widget_t::find_node_by_widget(ui::widget_id_t widget) const
	{
		for (const dock_node_t& node : _dock_nodes)
		{
			if (node.widget == widget)
				return &node;
		}
		SFG_ASSERT(false);
		return nullptr;
	}

	bool dock_widget_t::apply_payload_to_preview(dock_node_t& node, dock_preview_e preview, editor_panel_t* panel)
	{
		SFG_ASSERT(panel != nullptr);
		if (preview == dock_preview_e::center)
		{
			dock_node_add_panel(node, panel);
			return true;
		}
		return apply_payload_to_split_preview(node, preview, panel);
	}

	bool dock_widget_t::apply_payload_to_split_preview(dock_node_t& node, dock_preview_e preview, editor_panel_t* panel)
	{
		SFG_ASSERT(preview != dock_preview_e::center && preview != dock_preview_e::none);
		dock_node_add_panel(node, panel);
		return true;
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
				const bool is_root = !dock_widget._root_node.is_null() && &node == &dock_widget._dock_nodes.get(dock_widget._root_node);
				dock_widget.dock_node_remove_panel(node, identifier);
				if (node.panels.empty())
				{
					if (is_root && dock_widget._config.root_drag_out_behavior == dock_widget_root_drag_out_e::close_window)
						dock_widget._runtime->set_flag(window_runtime_flags_e::close_requested);
					else if (!is_root)
						dock_widget.collapse_empty_leaf_after_drag_out(node);
				}
				return;
			}
		}

		SFG_ASSERT(false);
	}

	bool dock_widget_t::is_leaf_tab_drag_out_allowed(editor_tab_area_t& tab_area, sid_t, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (dock_node_t& node : dock_widget._dock_nodes)
		{
			if (&node.tab_area == &tab_area)
			{
				const bool is_root = !dock_widget._root_node.is_null() && &node == &dock_widget._dock_nodes.get(dock_widget._root_node);
				if (!is_root)
					return true;
				if (dock_widget._config.root_drag_out_behavior == dock_widget_root_drag_out_e::close_window)
					return true;
				return node.panels.size() > 1;
			}
		}

		SFG_ASSERT(false);
		return false;
	}

	bool dock_widget_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::panel)
			return false;

		SFG_ASSERT(payload.user_ptr != nullptr);

		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (dock_node_t& node : dock_widget._dock_nodes)
		{
			if (node.node_type == dock_node_type_e::leaf && node.is_payload_over && node.hovered_preview != dock_preview_e::none)
				return dock_widget.apply_payload_to_preview(node, node.hovered_preview, static_cast<editor_panel_t*>(payload.user_ptr));
		}

		return false;
	}

	void dock_widget_t::on_payload_tick(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		dock_widget.update_dock_previews(payload, abs_mouse_pos);
	}

	void dock_widget_t::on_payload_end(const editor_payload_t&, void* user_data)
	{
		dock_widget_t& dock_widget		  = *static_cast<dock_widget_t*>(user_data);
		dock_widget._panel_payload_active = false;
		dock_widget.clear_dock_previews();
	}

	void dock_widget_t::draw_leaf_dock_previews(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		dock_widget_t&	   dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_node_t* node		   = dock_widget.find_node_by_widget(id);
		if (!dock_widget._panel_payload_active || !node->is_payload_over)
			return;

		const editor_theme_t& theme = editor_theme_t::get();
		const u64			  us	= static_cast<u64>(time_t::get_cpu_microseconds());
		const f32			  phase = static_cast<f32>(us % 600000ull) / 600000.0f;
		const f32			  pulse = easing_t::sinusodial(0.0f, 1.0f, phase);

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_accent1;
		rect.fill_color_b		 = theme.color_accent1;
		rect.outline_color		 = theme.color_accent1;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding_segs		 = 8;
		rect.fill_color_a.w		 = 0.18f;
		rect.fill_color_b.w		 = 0.18f;
		rect.outline_color.w	 = 0.85f;

		ui::ui_render_state_t state = {};
		state.pipeline				= theme.shader_dock_preview;

		const u32 draw_order = dock_widget._ui->get_tree().draw_order_const(id) + 1;
		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			vec4f_t preview = node->preview_rects[i];
			if (static_cast<dock_preview_e>(i) == node->hovered_preview)
				preview = expand_rect(preview, theme.item_spacing * (0.5f + pulse));

			canvas.add_rect({preview.x, preview.y}, {preview.x + preview.z, preview.y + preview.w}, rect, state, draw_order);
		}
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
