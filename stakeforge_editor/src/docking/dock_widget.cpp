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
#include <sfg/platform/process.hpp>
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

		ui.get_paint().set_custom(_root, draw_dock_previews, this);
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

		_ui						   = nullptr;
		_runtime				   = nullptr;
		_root					   = NULL_WIDGET;
		_root_node				   = {};
		_config					   = {};
		_hovered_dock_preview	   = DOCK_PREVIEW_NONE;
		_panel_payload_active	   = false;
		_panel_payload_over_widget = false;
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
		tab_config.window_minimized			= on_window_minimized;
		tab_config.window_maximized			= on_window_maximized;
		tab_config.window_closed			= on_window_closed;
		tab_config.show_window_buttons		= _config.show_window_buttons;
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

	bool dock_widget_t::is_window_drag_region(const vec2i16_t& pos) const
	{
		if (_root_node.is_null())
			return false;

		const dock_node_t& node = _dock_nodes.get(_root_node);
		if (node.node_type != dock_node_type_e::leaf)
			return false;

		const vec2f_t			p	= {static_cast<f32>(pos.x), static_cast<f32>(pos.y)};
		const ui::layout_out_t& out = _ui->get_tree().out(node.tab_area.get_root());
		if (p.x < out.pos.x || p.x > out.pos.x + out.size.x || p.y < out.pos.y || p.y > out.pos.y + out.size.y)
			return false;

		return !node.tab_area.is_over_tab(p);
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

	void dock_widget_t::update_dock_previews(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos)
	{
		_panel_payload_active	   = payload.type == editor_payload_type_e::panel;
		_panel_payload_over_widget = false;
		_hovered_dock_preview	   = DOCK_PREVIEW_NONE;

		if (!_panel_payload_active)
			return;

		SFG_ASSERT(_runtime != nullptr);

		const vec2f_t mouse = {
			static_cast<f32>(abs_mouse_pos.x - _runtime->pos.x),
			static_cast<f32>(abs_mouse_pos.y - _runtime->pos.y),
		};

		const ui::layout_out_t& out		  = _ui->get_tree().out(_root);
		const vec4f_t			root_rect = {out.pos.x, out.pos.y, out.size.x, out.size.y};
		_panel_payload_over_widget		  = contains(root_rect, mouse);
		if (!_panel_payload_over_widget)
			return;

		const editor_theme_t& theme	   = editor_theme_t::get();
		const f32			  margin   = theme.item_height;
		const f32			  x		   = out.pos.x + margin;
		const f32			  y		   = out.pos.y + margin;
		const f32			  w		   = math::max(theme.item_height * 4.0f, out.size.x - margin * 2.0f);
		const f32			  h		   = math::max(theme.item_height * 4.0f, out.size.y - margin * 2.0f);
		const f32			  edge_w   = math::min(w * 0.09f, theme.item_height * 3.5f);
		const f32			  edge_h   = math::min(h * 0.09f, theme.item_height * 2.75f);
		const f32			  long_w   = w * 0.16f;
		const f32			  long_h   = h * 0.16f;
		const f32			  center_w = math::min(w * 0.11f, theme.item_height * 3.5f);
		const f32			  center_h = math::min(h * 0.11f, theme.item_height * 2.75f);

		_dock_preview_rects[0] = {x + (w - long_w) * 0.5f, y, long_w, edge_h};
		_dock_preview_rects[1] = {x, y + (h - long_h) * 0.5f, edge_w, long_h};
		_dock_preview_rects[2] = {x + (w - long_w) * 0.5f, y + h - edge_h, long_w, edge_h};
		_dock_preview_rects[3] = {x + w - edge_w, y + (h - long_h) * 0.5f, edge_w, long_h};
		_dock_preview_rects[4] = {x + (w - center_w) * 0.5f, y + (h - center_h) * 0.5f, center_w, center_h};

		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			if (contains(_dock_preview_rects[i], mouse))
			{
				_hovered_dock_preview = i;
				return;
			}
		}
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

	void dock_widget_t::on_window_minimized(editor_tab_area_t&, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		SFG_ASSERT(dock_widget._runtime != nullptr);
		process::minimize_window(dock_widget._runtime->window_handle);
	}

	void dock_widget_t::on_window_maximized(editor_tab_area_t&, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		SFG_ASSERT(dock_widget._runtime != nullptr);
		process::toggle_maximize_window(dock_widget._runtime->window_handle);
	}

	void dock_widget_t::on_window_closed(editor_tab_area_t&, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		SFG_ASSERT(dock_widget._runtime != nullptr);
		dock_widget._runtime->set_flag(window_runtime_flags_e::close_requested);
	}

	bool dock_widget_t::on_payload_drop(const editor_payload_t&, void*)
	{
		return false;
	}

	void dock_widget_t::on_payload_tick(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		dock_widget.update_dock_previews(payload, abs_mouse_pos);
	}

	void dock_widget_t::on_payload_end(const editor_payload_t&, void* user_data)
	{
		dock_widget_t& dock_widget			   = *static_cast<dock_widget_t*>(user_data);
		dock_widget._panel_payload_active	   = false;
		dock_widget._panel_payload_over_widget = false;
		dock_widget._hovered_dock_preview	   = DOCK_PREVIEW_NONE;
	}

	void dock_widget_t::draw_dock_previews(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		if (!dock_widget._panel_payload_active || !dock_widget._panel_payload_over_widget)
			return;

		const editor_theme_t& theme = editor_theme_t::get();
		const u64			  us	= static_cast<u64>(time_t::get_cpu_microseconds());
		const f32			  phase = static_cast<f32>(us % 800000ull) / 800000.0f;
		const f32			  pulse = easing_t::sinusodial(0.0f, 1.0f, phase);

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_accent1;
		rect.fill_color_b		 = theme.color_accent1;
		rect.outline_color		 = theme.color_accent1;
		rect.rounding			 = theme.item_spacing;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding_segs		 = 8;
		rect.fill_color_a.w		 = 0.18f;
		rect.fill_color_b.w		 = 0.18f;
		rect.outline_color.w	 = 0.85f;

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;

		const u32 draw_order = dock_widget._ui->get_tree().draw_order_const(id) + 1;
		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			vec4f_t preview = dock_widget._dock_preview_rects[i];
			if (i == dock_widget._hovered_dock_preview)
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
