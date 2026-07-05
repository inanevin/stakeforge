/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/docking/dock_widget.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/panels/editor_panel_factory.hpp"
#include "ui/panels/editor_panel_types.hpp"
#include "ui/panels/editor_theme.hpp"
#include "editor_app.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#define DOCK_PREVIEW_MARGIN_ITEM_HEIGHTS		1.0f
#define DOCK_PREVIEW_MIN_AXIS_ITEM_HEIGHTS		4.0f
#define DOCK_PREVIEW_RECT_ITEM_HEIGHTS			2.75f
#define DOCK_PREVIEW_GAP_ITEM_SPACINGS			1.5f
#define DOCK_PREVIEW_HOVER_EXPAND_ITEM_SPACINGS 0.5f
#define DOCK_PREVIEW_PULSE_US					600000ull
#define DOCK_SPLIT_INITIAL_VALUE				0.5f
#define DOCK_SPLIT_BORDER_THICKNESS_MULT		2.0f
#define DOCK_SPLIT_MIN_LEAF_ITEM_HEIGHTS		16.0f

namespace sfg
{
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
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::none;

		editor_payload_controller_t::get().register_listener(on_payload_drop, on_payload_tick, on_payload_end, this);
	}

	void dock_widget_t::uninit()
	{
		if (!_root_node.is_null())
			destroy_dock_node(_root_node);

		_ui->deallocate_widget(_root);
		editor_payload_controller_t::get().unregister_listener(this);

		_ui					  = nullptr;
		_runtime			  = nullptr;
		_root				  = NULL_WIDGET;
		_root_node			  = {};
		_config				  = {};
		_panel_payload_active = false;
		_dock_nodes.clear();
		_dock_borders.clear();
	}

	void dock_widget_t::init_leaf_node_content(dock_node_t& node)
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& widget_in = tree.in(node.widget);
		widget_in.flags			   = ui::wf_visible;
		widget_in.flow			   = ui::flow_e::column;
		widget_in.child_spacing	   = 0.0f;
		widget_in.child_margins	   = {0.0f, 0.0f, 0.0f, 0.0f};

		paint.set_custom(node.widget, draw_leaf_dock_previews, this);

		editor_tab_area_config_t tab_config = {};
		tab_config.tab_switched				= on_leaf_tab_switched;
		tab_config.tab_dragged_out			= on_leaf_tab_dragged_out;
		tab_config.tab_removed				= on_leaf_tab_closed;
		tab_config.drag_out_allowed			= is_leaf_tab_drag_out_allowed;
		tab_config.close_allowed			= is_leaf_tab_close_allowed;
		tab_config.user_data				= this;
		tab_config.can_drag_out				= true;
		node.tab_area.init(ui, node.widget, tab_config);

		node.body = ui.allocate_widget();
		ui.set_widget_debug_name(node.body, "dock_node_body");
		tree.attach(node.widget, node.body);

		ui::layout_in_t& body_in = tree.in(node.body);
		body_in.flags			 = ui::wf_visible;
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
	}

	dock_node_handle_t dock_widget_t::create_leaf_node(ui::widget_id_t parent)
	{
		ui::ui_context&			 ui		= *_ui;
		ui::layout_tree_t&		 tree	= ui.get_tree();
		const dock_node_handle_t handle = alloc_dock_node();
		dock_node_t&			 node	= _dock_nodes.get(handle);
		node.node_type					= dock_node_type_e::leaf;

		node.widget = ui.allocate_widget();
		ui.set_widget_debug_name(node.widget, "dock_node_leaf");
		tree.attach(parent, node.widget);

		ui::layout_in_t& widget_in = tree.in(node.widget);
		widget_in.flags			   = ui::wf_visible;
		widget_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		widget_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		widget_in.size_value	   = {1.0f, 1.0f};

		init_leaf_node_content(node);

		return handle;
	}

	dock_node_handle_t dock_widget_t::create_split_node(ui::widget_id_t parent, dock_split_direction_e direction, f32 split_value)
	{
		ui::ui_context&			 ui		= *_ui;
		ui::layout_tree_t&		 tree	= ui.get_tree();
		const dock_node_handle_t handle = alloc_dock_node();
		dock_node_t&			 node	= _dock_nodes.get(handle);
		node.node_type					= dock_node_type_e::split;
		node.split_direction			= direction;
		node.split_value				= split_value;

		node.widget = ui.allocate_widget();
		ui.set_widget_debug_name(node.widget, "dock_node_split");
		tree.attach(parent, node.widget);

		ui::layout_in_t& widget_in = tree.in(node.widget);
		widget_in.flags			   = ui::wf_visible;
		widget_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		widget_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		widget_in.size_value	   = {1.0f, 1.0f};

		return handle;
	}

	void dock_widget_t::set_root_node(dock_node_handle_t handle)
	{
		SFG_ASSERT(_dock_nodes.is_valid(handle));
		_root_node = handle;
	}

	nlohmann::json dock_widget_t::to_json() const
	{
		nlohmann::json j = nlohmann::json::object();
		j["version"]	 = 1;
		if (!_root_node.is_null())
			j["root"] = dock_node_to_json(_dock_nodes.get(_root_node));
		return j;
	}

	bool dock_widget_t::from_json(const nlohmann::json& j)
	{
		if (!j.is_object())
			return false;

		if (!j.contains("root") || !j.at("root").is_object())
			return false;

		if (!_root_node.is_null())
		{
			destroy_dock_node(_root_node);
			_root_node = {};
		}

		const nlohmann::json& root = j.at("root");
		_root_node				   = dock_node_from_json(_root, root);
		return !_root_node.is_null();
	}

	editor_panel_t* dock_widget_t::find_panel(editor_panel_type_e type) const
	{
		for (const dock_node_t& node : _dock_nodes)
		{
			for (editor_panel_t* panel : node.panels)
			{
				if (panel->get_type() == type)
					return panel;
			}
		}
		return nullptr;
	}

	bool dock_widget_t::select_panel(editor_panel_t* panel)
	{
		SFG_ASSERT(panel != nullptr);
		for (dock_node_t& node : _dock_nodes)
		{
			if (node.node_type != dock_node_type_e::leaf)
				continue;

			for (editor_panel_t* candidate : node.panels)
			{
				if (candidate == panel)
				{
					node.tab_area.select_tab(TO_SID(panel->get_title()));
					return true;
				}
			}
		}
		return false;
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
		node.panels.push_back(panel);
		panel->assign(*_ui, node.body);
		node.tab_area.select_tab(TO_SID(panel->get_title()));
	}

	bool dock_widget_t::refresh_panel_title(editor_panel_t* panel, sid_t old_identifier)
	{
		SFG_ASSERT(panel != nullptr);

		for (dock_node_t& node : _dock_nodes)
		{
			if (node.node_type != dock_node_type_e::leaf)
				continue;

			for (editor_panel_t* candidate : node.panels)
			{
				if (candidate == panel)
				{
					node.tab_area.rename_tab(old_identifier, panel->get_title());
					return true;
				}
			}
		}
		return false;
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

	void dock_widget_t::destroy_dock_node(dock_node_handle_t handle)
	{
		SFG_ASSERT(_dock_nodes.is_valid(handle));
		dock_node_t& node = _dock_nodes.get(handle);
		if (node.node_type == dock_node_type_e::split)
		{
			destroy_dock_node(node.split_negative);
			destroy_dock_node(node.split_positive);
			destroy_split_border(node);
		}
		else
		{
			for (editor_panel_t* panel : node.panels)
			{
				panel->uninit();
				editor_panel_factory_t::delete_panel(panel);
			}
			node.panels.clear();
			node.tab_area.uninit();
		}

		_ui->deallocate_widget(node.widget);
		free_dock_node(handle);
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
		for (rectf_t& rect : node.preview_rects)
			rect = {};

		const ui::layout_out_t& out					 = _ui->get_tree().out(node.widget);
		const editor_theme_t&	theme				 = editor_theme_t::get();
		const f32				margin				 = theme.item_height * DOCK_PREVIEW_MARGIN_ITEM_HEIGHTS;
		const f32				x					 = out.pos.x + margin;
		const f32				y					 = out.pos.y + margin;
		const f32				w					 = math::max(theme.item_height * DOCK_PREVIEW_MIN_AXIS_ITEM_HEIGHTS, out.size.x - margin * 2.0f);
		const f32				h					 = math::max(theme.item_height * DOCK_PREVIEW_MIN_AXIS_ITEM_HEIGHTS, out.size.y - margin * 2.0f);
		const f32				preview_w			 = theme.item_height * DOCK_PREVIEW_RECT_ITEM_HEIGHTS;
		const f32				preview_h			 = theme.item_height * DOCK_PREVIEW_RECT_ITEM_HEIGHTS;
		const f32				gap					 = theme.item_spacing * DOCK_PREVIEW_GAP_ITEM_SPACINGS;
		const f32				center_x			 = x + w * 0.5f;
		const f32				center_y			 = y + h * 0.5f;
		const f32				base_x				 = center_x - preview_w * 0.5f;
		const f32				base_y				 = center_y - preview_h * 0.5f;
		const bool				can_split_horizontal = can_split_leaf(node, dock_preview_e::left);
		const bool				can_split_vertical	 = can_split_leaf(node, dock_preview_e::top);

		if (can_split_vertical)
		{
			node.preview_rects[static_cast<u32>(dock_preview_e::top)]	 = {base_x, base_y - preview_h - gap, preview_w, preview_h};
			node.preview_rects[static_cast<u32>(dock_preview_e::bottom)] = {base_x, base_y + preview_h + gap, preview_w, preview_h};
		}
		if (can_split_horizontal)
		{
			node.preview_rects[static_cast<u32>(dock_preview_e::left)]	= {base_x - preview_w - gap, base_y, preview_w, preview_h};
			node.preview_rects[static_cast<u32>(dock_preview_e::right)] = {base_x + preview_w + gap, base_y, preview_w, preview_h};
		}
		node.preview_rects[static_cast<u32>(dock_preview_e::center)] = {base_x, base_y, preview_w, preview_h};

		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			const rectf_t& preview = node.preview_rects[i];
			if (preview.w > 0.0f && preview.h > 0.0f && preview.contains(mouse))
			{
				node.hovered_preview = static_cast<dock_preview_e>(i);
				return;
			}
		}
	}

	void dock_widget_t::collapse_empty_leaf_after_drag_out(dock_node_t& node)
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);
		SFG_ASSERT(node.panels.empty());

		const dock_node_handle_t empty_handle	= find_node_handle(node);
		dock_node_handle_t		 parent_handle	= {};
		bool					 empty_negative = false;
		SFG_ASSERT(find_parent_split(empty_handle, parent_handle, empty_negative));

		dock_node_t&			 parent			= _dock_nodes.get(parent_handle);
		const dock_node_handle_t sibling_handle = empty_negative ? parent.split_positive : parent.split_negative;
		dock_node_t&			 sibling		= _dock_nodes.get(sibling_handle);

		if (sibling.node_type == dock_node_type_e::leaf)
		{
			const sid_t						active_tab = sibling.tab_area.get_active_tab();
			frame_vector_t<editor_panel_t*> panels(sibling.panels.begin(), sibling.panels.end());
			sibling.panels.clear();
			for (editor_panel_t* panel : panels)
				panel->deassign();

			destroy_split_border(parent);
			destroy_dock_node(empty_handle);
			destroy_dock_node(sibling_handle);

			dock_node_t& collapsed	  = _dock_nodes.get(parent_handle);
			collapsed.node_type		  = dock_node_type_e::leaf;
			collapsed.split_negative  = {};
			collapsed.split_positive  = {};
			collapsed.hovered_preview = dock_preview_e::none;
			collapsed.is_payload_over = false;
			init_leaf_node_content(collapsed);
			for (editor_panel_t* panel : panels)
				dock_node_add_panel(collapsed, panel);
			if (active_tab != 0)
				collapsed.tab_area.select_tab(active_tab);
			return;
		}

		const dock_node_handle_t	 negative		= sibling.split_negative;
		const dock_node_handle_t	 positive		= sibling.split_positive;
		const dock_border_handle_t	 sibling_border = sibling.border;
		const dock_split_direction_e direction		= sibling.split_direction;
		const f32					 split_value	= sibling.split_value;
		const ui::widget_id_t		 sibling_widget = sibling.widget;

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.attach(parent.widget, _dock_nodes.get(negative).widget);
		tree.attach(parent.widget, _dock_borders.get(sibling_border).widget);
		tree.attach(parent.widget, _dock_nodes.get(positive).widget);

		destroy_split_border(parent);
		destroy_dock_node(empty_handle);
		_ui->deallocate_widget(sibling_widget);
		free_dock_node(sibling_handle);

		dock_node_t& collapsed					= _dock_nodes.get(parent_handle);
		collapsed.node_type						= dock_node_type_e::split;
		collapsed.split_direction				= direction;
		collapsed.split_value					= split_value;
		collapsed.border						= sibling_border;
		collapsed.split_negative				= negative;
		collapsed.split_positive				= positive;
		collapsed.hovered_preview				= dock_preview_e::none;
		collapsed.is_payload_over				= false;
		_dock_borders.get(sibling_border).split = parent_handle;
		configure_split_child_layout(collapsed);
	}

	dock_node_handle_t dock_widget_t::find_node_handle(const dock_node_t& node) const
	{
		for (u16 i = 0; i < _dock_nodes.head(); ++i)
		{
			if (_dock_nodes.is_active(i) && &_dock_nodes.get(i) == &node)
				return _dock_nodes.get_handle(i);
		}

		SFG_ASSERT(false);
		return {};
	}

	bool dock_widget_t::find_parent_split(dock_node_handle_t child, dock_node_handle_t& out_parent, bool& out_is_negative) const
	{
		for (u16 i = 0; i < _dock_nodes.head(); ++i)
		{
			if (!_dock_nodes.is_active(i))
				continue;

			const dock_node_t& node = _dock_nodes.get(i);
			if (node.node_type != dock_node_type_e::split)
				continue;

			if (node.split_negative == child)
			{
				out_parent		= _dock_nodes.get_handle(i);
				out_is_negative = true;
				return true;
			}
			if (node.split_positive == child)
			{
				out_parent		= _dock_nodes.get_handle(i);
				out_is_negative = false;
				return true;
			}
		}

		return false;
	}

	void dock_widget_t::init_split_border(dock_node_t& split_node, dock_node_handle_t split_handle)
	{
		SFG_ASSERT(split_node.node_type == dock_node_type_e::split);
		SFG_ASSERT(split_node.border.is_null());

		ui::ui_context&			   ui	  = *_ui;
		ui::layout_tree_t&		   tree	  = ui.get_tree();
		const dock_border_handle_t handle = alloc_dock_border();
		dock_border_t&			   border = _dock_borders.get(handle);
		border.split					  = split_handle;
		border.widget					  = ui.allocate_widget();
		border.is_dragging				  = false;
		split_node.border				  = handle;

		ui.set_widget_debug_name(border.widget, "dock_split_border");
		tree.attach(split_node.widget, border.widget);

		ui::layout_in_t& in = tree.in(border.widget);
		in.flags			= ui::wf_visible | ui::wf_input;

		ui::listener_bundle_t listener = {};
		listener.on_hover_enter		   = on_split_border_hover_enter;
		listener.on_hover_exit		   = on_split_border_hover_exit;
		listener.on_hover_move		   = on_split_border_hover_move;
		listener.on_drag_begin		   = on_split_border_drag_begin;
		listener.on_drag			   = on_split_border_drag;
		listener.on_drag_end		   = on_split_border_drag_end;
		listener.user_data			   = this;
		ui.get_input().set_listener(border.widget, listener);
		ui.get_paint().set_custom(border.widget, draw_split_border, this);
	}

	void dock_widget_t::destroy_split_border(dock_node_t& split_node)
	{
		SFG_ASSERT(split_node.node_type == dock_node_type_e::split);
		SFG_ASSERT(!split_node.border.is_null());

		dock_border_t& border = _dock_borders.get(split_node.border);
		_ui->deallocate_widget(border.widget);
		free_dock_border(split_node.border);
		split_node.border = {};
	}

	void dock_widget_t::apply_split_border_drag(dock_border_t& border, const vec2f_t& pos)
	{
		SFG_ASSERT(_dock_nodes.is_valid(border.split));

		dock_node_t& split_node = _dock_nodes.get(border.split);
		SFG_ASSERT(split_node.node_type == dock_node_type_e::split);

		const ui::layout_out_t& out	  = _ui->get_tree().out(split_node.widget);
		const editor_theme_t&	theme = editor_theme_t::get();
		const f32				axis  = split_node.split_direction == dock_split_direction_e::horizontal ? out.size.x : out.size.y;
		const f32				mouse = split_node.split_direction == dock_split_direction_e::horizontal ? pos.x - out.pos.x : pos.y - out.pos.y;
		SFG_ASSERT(axis > 0.0f);
		const f32 min_size	  = theme.item_height * DOCK_SPLIT_MIN_LEAF_ITEM_HEIGHTS;
		const f32 border_size = theme.border_thickness * DOCK_SPLIT_BORDER_THICKNESS_MULT;
		const f32 min_frac	  = min_size / axis;
		const f32 max_frac	  = (axis - border_size - min_size) / axis;

		split_node.split_value = max_frac > min_frac ? math::clamp(mouse / axis, min_frac, max_frac) : DOCK_SPLIT_INITIAL_VALUE;
		configure_split_child_layout(split_node);
	}

	void dock_widget_t::configure_split_child_layout(dock_node_t& split_node)
	{
		SFG_ASSERT(split_node.node_type == dock_node_type_e::split);
		SFG_ASSERT(!split_node.border.is_null());

		ui::layout_tree_t&	  tree	   = _ui->get_tree();
		dock_node_t&		  negative = _dock_nodes.get(split_node.split_negative);
		dock_node_t&		  positive = _dock_nodes.get(split_node.split_positive);
		dock_border_t&		  border   = _dock_borders.get(split_node.border);
		const editor_theme_t& theme	   = editor_theme_t::get();

		ui::layout_in_t& split_in = tree.in(split_node.widget);
		split_in.flow			  = split_node.split_direction == dock_split_direction_e::horizontal ? ui::flow_e::row : ui::flow_e::column;
		split_in.child_spacing	  = 0.0f;

		ui::layout_in_t& negative_in = tree.in(negative.widget);
		ui::layout_in_t& border_in	 = tree.in(border.widget);
		ui::layout_in_t& positive_in = tree.in(positive.widget);
		if (split_node.split_direction == dock_split_direction_e::horizontal)
		{
			negative_in.size_mode_x = ui::axis_mode_e::parent_relative;
			negative_in.size_mode_y = ui::axis_mode_e::parent_relative;
			negative_in.size_value	= {split_node.split_value, 1.0f};
			border_in.size_mode_x	= ui::axis_mode_e::fixed;
			border_in.size_mode_y	= ui::axis_mode_e::parent_relative;
			border_in.size_value	= {theme.border_thickness * DOCK_SPLIT_BORDER_THICKNESS_MULT, 1.0f};
			positive_in.size_mode_x = ui::axis_mode_e::fill;
			positive_in.size_mode_y = ui::axis_mode_e::parent_relative;
			positive_in.size_value	= {1.0f, 1.0f};
		}
		else
		{
			negative_in.size_mode_x = ui::axis_mode_e::parent_relative;
			negative_in.size_mode_y = ui::axis_mode_e::parent_relative;
			negative_in.size_value	= {1.0f, split_node.split_value};
			border_in.size_mode_x	= ui::axis_mode_e::parent_relative;
			border_in.size_mode_y	= ui::axis_mode_e::fixed;
			border_in.size_value	= {1.0f, theme.border_thickness * DOCK_SPLIT_BORDER_THICKNESS_MULT};
			positive_in.size_mode_x = ui::axis_mode_e::parent_relative;
			positive_in.size_mode_y = ui::axis_mode_e::fill;
			positive_in.size_value	= {1.0f, 1.0f};
		}
	}

	bool dock_widget_t::can_split_leaf(const dock_node_t& node, dock_preview_e preview) const
	{
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);

		if (preview == dock_preview_e::center)
			return true;

		const ui::layout_out_t& out			= _ui->get_tree().out(node.widget);
		const editor_theme_t&	theme		= editor_theme_t::get();
		const f32				min_size	= theme.item_height * DOCK_SPLIT_MIN_LEAF_ITEM_HEIGHTS;
		const f32				border_size = theme.border_thickness * DOCK_SPLIT_BORDER_THICKNESS_MULT;
		const bool				horizontal	= preview == dock_preview_e::left || preview == dock_preview_e::right;
		const f32				axis		= horizontal ? out.size.x : out.size.y;
		return axis >= min_size * 2.0f + border_size;
	}

	bool dock_widget_t::split_leaf_node(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel)
	{
		SFG_ASSERT(_dock_nodes.is_valid(handle));
		SFG_ASSERT(panel != nullptr);
		SFG_ASSERT(preview != dock_preview_e::center && preview != dock_preview_e::none);

		dock_node_t& node = _dock_nodes.get(handle);
		SFG_ASSERT(node.node_type == dock_node_type_e::leaf);
		SFG_ASSERT(can_split_leaf(node, preview));

		const bool						split_horizontal = preview == dock_preview_e::left || preview == dock_preview_e::right;
		const bool						new_is_negative	 = preview == dock_preview_e::left || preview == dock_preview_e::top;
		const sid_t						active_tab		 = node.tab_area.get_active_tab();
		frame_vector_t<editor_panel_t*> existing_panels(node.panels.begin(), node.panels.end());
		node.panels.clear();
		for (editor_panel_t* existing_panel : existing_panels)
			existing_panel->deassign();

		node.tab_area.uninit();
		_ui->deallocate_widget(node.body);
		node.body			 = NULL_WIDGET;
		node.node_type		 = dock_node_type_e::split;
		node.split_direction = split_horizontal ? dock_split_direction_e::horizontal : dock_split_direction_e::vertical;
		node.split_value	 = DOCK_SPLIT_INITIAL_VALUE;
		node.hovered_preview = dock_preview_e::none;
		node.is_payload_over = false;

		const ui::widget_id_t	 split_widget = node.widget;
		const dock_node_handle_t negative	  = create_leaf_node(split_widget);
		init_split_border(_dock_nodes.get(handle), handle);
		const dock_node_handle_t positive = create_leaf_node(split_widget);

		dock_node_t& split_node				   = _dock_nodes.get(handle);
		split_node.split_negative			   = negative;
		split_node.split_positive			   = positive;
		const dock_node_handle_t new_leaf	   = new_is_negative ? negative : positive;
		const dock_node_handle_t existing_leaf = new_is_negative ? positive : negative;

		configure_split_child_layout(split_node);

		dock_node_t& existing_node = _dock_nodes.get(existing_leaf);
		for (editor_panel_t* existing_panel : existing_panels)
			dock_node_add_panel(existing_node, existing_panel);
		if (active_tab != 0)
			existing_node.tab_area.select_tab(active_tab);

		dock_node_add_panel(_dock_nodes.get(new_leaf), panel);
		return true;
	}

	nlohmann::json dock_widget_t::dock_node_to_json(const dock_node_t& node) const
	{
		nlohmann::json j = nlohmann::json::object();
		j["type"]		 = dock_node_type_to_string(node.node_type);

		if (node.node_type == dock_node_type_e::split)
		{
			j["direction"]	 = dock_split_direction_to_string(node.split_direction);
			j["split_value"] = node.split_value;
			j["negative"]	 = dock_node_to_json(_dock_nodes.get(node.split_negative));
			j["positive"]	 = dock_node_to_json(_dock_nodes.get(node.split_positive));
			return j;
		}

		nlohmann::json panels = nlohmann::json::array();
		for (editor_panel_t* panel : node.panels)
		{
			nlohmann::json panel_data = nlohmann::json::object();
			panel->serialize(panel_data);

			nlohmann::json panel_json = nlohmann::json::object();
			panel_json["type"]		  = editor_panel_type_to_string(panel->get_type());
			panel_json["data"]		  = panel_data;
			panels.push_back(panel_json);
		}

		j["panels"]			   = panels;
		const sid_t active_tab = node.tab_area.get_active_tab();
		for (editor_panel_t* panel : node.panels)
		{
			if (TO_SID(panel->get_title()) == active_tab)
			{
				j["active_panel"] = panel->get_title();
				break;
			}
		}
		return j;
	}

	dock_node_handle_t dock_widget_t::dock_node_from_json(ui::widget_id_t parent, const nlohmann::json& j)
	{
		const string_t		   type_name = j.value<string_t>("type", "leaf");
		const dock_node_type_e type		 = dock_node_type_from_string(type_name.c_str());
		if (type == dock_node_type_e::split)
		{
			const string_t				 direction_name = j.value<string_t>("direction", "horizontal");
			const dock_split_direction_e direction		= dock_split_direction_from_string(direction_name.c_str());
			const f32					 split_value	= math::clamp(j.value<f32>("split_value", DOCK_SPLIT_INITIAL_VALUE), 0.0f, 1.0f);
			const dock_node_handle_t	 handle			= create_split_node(parent, direction, split_value);
			dock_node_t&				 node			= _dock_nodes.get(handle);

			const nlohmann::json negative = j.value("negative", nlohmann::json{{"type", "leaf"}});
			const nlohmann::json positive = j.value("positive", nlohmann::json{{"type", "leaf"}});
			node.split_negative			  = dock_node_from_json(node.widget, negative);
			init_split_border(node, handle);
			node.split_positive = dock_node_from_json(node.widget, positive);
			configure_split_child_layout(node);
			return handle;
		}

		const dock_node_handle_t handle = create_leaf_node(parent);
		dock_node_t&			 node	= _dock_nodes.get(handle);
		const nlohmann::json	 panels = j.value("panels", nlohmann::json::array());
		for (const nlohmann::json& panel_json : panels)
		{
			const string_t			  type_name = panel_json.value<string_t>("type", {});
			const editor_panel_type_e type		= editor_panel_type_from_string(type_name.c_str());
			if (type == editor_panel_type_e::max)
				continue;

			editor_panel_t* panel = editor_panel_factory_t::create_panel(type);
			panel->deserialize(panel_json.value("data", nlohmann::json::object()));
			dock_node_add_panel(node, panel);
		}

		const string_t active_panel = j.value<string_t>("active_panel", {});
		for (editor_panel_t* panel : node.panels)
		{
			if (TO_SID(panel->get_title()) == TO_SID(active_panel))
			{
				node.tab_area.select_tab(TO_SID(panel->get_title()));
				break;
			}
		}
		return handle;
	}

	dock_node_t* dock_widget_t::find_leaf_at(const vec2f_t& mouse)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (dock_node_t& node : _dock_nodes)
		{
			if (node.node_type == dock_node_type_e::leaf)
			{
				const ui::layout_out_t& out	 = tree.out(node.widget);
				const rectf_t			rect = {out.pos.x, out.pos.y, out.size.x, out.size.y};
				if (rect.contains(mouse))
					return &node;
			}
		}
		return nullptr;
	}

	dock_border_t* dock_widget_t::find_border_by_widget(ui::widget_id_t widget)
	{
		for (dock_border_t& border : _dock_borders)
		{
			if (border.widget == widget)
				return &border;
		}
		SFG_ASSERT(false);
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

	bool dock_widget_t::apply_payload_to_preview(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel)
	{
		SFG_ASSERT(panel != nullptr);
		if (preview == dock_preview_e::center)
		{
			dock_node_add_panel(_dock_nodes.get(handle), panel);
			return true;
		}
		return apply_payload_to_split_preview(handle, preview, panel);
	}

	bool dock_widget_t::apply_payload_to_split_preview(dock_node_handle_t handle, dock_preview_e preview, editor_panel_t* panel)
	{
		SFG_ASSERT(preview != dock_preview_e::center && preview != dock_preview_e::none);
		return split_leaf_node(handle, preview, panel);
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

	bool dock_widget_t::is_leaf_tab_close_allowed(editor_tab_area_t& tab_area, sid_t, void* user_data)
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

	void dock_widget_t::on_leaf_tab_closed(editor_tab_area_t& tab_area, sid_t identifier, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (dock_node_t& node : dock_widget._dock_nodes)
		{
			if (&node.tab_area == &tab_area)
			{
				const bool is_root = !dock_widget._root_node.is_null() && &node == &dock_widget._dock_nodes.get(dock_widget._root_node);
				for (auto it = node.panels.begin(); it != node.panels.end(); ++it)
				{
					editor_panel_t* panel = *it;
					if (TO_SID(panel->get_title()) == identifier)
					{
						panel->deassign();
						panel->uninit();
						editor_panel_factory_t::delete_panel(panel);
						node.panels.erase(it);
						break;
					}
				}
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

	bool dock_widget_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::panel)
			return false;

		SFG_ASSERT(payload.user_ptr != nullptr);

		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		for (u16 i = 0; i < dock_widget._dock_nodes.head(); ++i)
		{
			if (!dock_widget._dock_nodes.is_active(i))
				continue;

			dock_node_t& node = dock_widget._dock_nodes.get(i);
			if (node.node_type == dock_node_type_e::leaf && node.is_payload_over && node.hovered_preview != dock_preview_e::none)
				return dock_widget.apply_payload_to_preview(dock_widget._dock_nodes.get_handle(i), node.hovered_preview, static_cast<editor_panel_t*>(payload.user_ptr));
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

	void dock_widget_t::on_split_border_hover_enter(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		dock_widget_t&		  dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_border_t*  border	  = dock_widget.find_border_by_widget(id);
		const dock_node_t&	  split_node  = dock_widget._dock_nodes.get(border->split);
		window_cursor_state_e cursor	  = split_node.split_direction == dock_split_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
		process::set_cursor_state(cursor);
	}

	void dock_widget_t::on_split_border_hover_exit(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		dock_widget_t&		 dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_border_t* border		 = dock_widget.find_border_by_widget(id);
		if (!border->is_dragging)
			process::set_cursor_state(window_cursor_state_e::arrow);
	}

	void dock_widget_t::on_split_border_hover_move(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		dock_widget_t&		  dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_border_t*  border	  = dock_widget.find_border_by_widget(id);
		const dock_node_t&	  split_node  = dock_widget._dock_nodes.get(border->split);
		window_cursor_state_e cursor	  = split_node.split_direction == dock_split_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
		process::set_cursor_state(cursor);
	}

	void dock_widget_t::on_split_border_drag_begin(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		dock_widget_t&		  dock_widget = *static_cast<dock_widget_t*>(user_data);
		dock_border_t*		  border	  = dock_widget.find_border_by_widget(id);
		const dock_node_t&	  split_node  = dock_widget._dock_nodes.get(border->split);
		window_cursor_state_e cursor	  = split_node.split_direction == dock_split_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
		border->is_dragging				  = true;
		process::set_cursor_state(cursor);
		dock_widget.apply_split_border_drag(*border, pos);
	}

	void dock_widget_t::on_split_border_drag(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		dock_widget_t&		  dock_widget = *static_cast<dock_widget_t*>(user_data);
		dock_border_t*		  border	  = dock_widget.find_border_by_widget(id);
		const dock_node_t&	  split_node  = dock_widget._dock_nodes.get(border->split);
		window_cursor_state_e cursor	  = split_node.split_direction == dock_split_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
		process::set_cursor_state(cursor);
		dock_widget.apply_split_border_drag(*border, pos);
	}

	void dock_widget_t::on_split_border_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		dock_widget_t& dock_widget = *static_cast<dock_widget_t*>(user_data);
		dock_border_t* border	   = dock_widget.find_border_by_widget(id);
		dock_widget.apply_split_border_drag(*border, pos);
		border->is_dragging = false;
		if (router.get_hovered() == id)
		{
			const dock_node_t&	  split_node = dock_widget._dock_nodes.get(border->split);
			window_cursor_state_e cursor	 = split_node.split_direction == dock_split_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
			process::set_cursor_state(cursor);
		}
		else
		{
			process::set_cursor_state(window_cursor_state_e::arrow);
		}
	}

	void dock_widget_t::draw_split_border(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		dock_widget_t&			dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_border_t*	border		= dock_widget.find_border_by_widget(id);
		const editor_theme_t&	theme		= editor_theme_t::get();
		const ui::layout_out_t& out			= dock_widget._ui->get_tree().out(id);

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = border->is_dragging || dock_widget._ui->get_input().get_hovered() == id ? theme.color_frame_light : theme.color_frame;
		rect.fill_color_b		 = rect.fill_color_a;

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;
		canvas.add_rect({out.pos.x, out.pos.y}, {out.pos.x + out.size.x, out.pos.y + out.size.y}, rect, state, dock_widget._ui->get_tree().draw_order_const(id));
	}

	void dock_widget_t::draw_leaf_dock_previews(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		dock_widget_t&	   dock_widget = *static_cast<dock_widget_t*>(user_data);
		const dock_node_t* node		   = dock_widget.find_node_by_widget(id);
		if (!dock_widget._panel_payload_active || !node->is_payload_over)
			return;

		const editor_theme_t& theme = editor_theme_t::get();
		const u64			  us	= static_cast<u64>(time_t::get_cpu_microseconds());
		const f32			  phase = static_cast<f32>(us % DOCK_PREVIEW_PULSE_US) / static_cast<f32>(DOCK_PREVIEW_PULSE_US);
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
		state.pipeline				= theme.shader_glitch_rect;

		const u32 draw_order = dock_widget._ui->get_tree().draw_order_const(id) + 100;
		for (u32 i = 0; i < DOCK_PREVIEW_COUNT; ++i)
		{
			rectf_t preview = node->preview_rects[i];
			if (preview.w <= 0.0f || preview.h <= 0.0f)
				continue;

			if (static_cast<dock_preview_e>(i) == node->hovered_preview)
				preview = preview.expand(theme.item_spacing * (DOCK_PREVIEW_HOVER_EXPAND_ITEM_SPACINGS + pulse));

			canvas.add_rect({preview.x, preview.y}, {preview.x + preview.w, preview.y + preview.h}, rect, state, draw_order);
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
