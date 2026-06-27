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
#include "ui/widgets/editor_tab_area.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	void editor_tab_area_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_tab_area_config_t& config)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui							= &ui;
		_config						= config;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_tab_area");
		tree.attach(parent, _root);

		ui::layout_in_t& in = tree.in(_root);
		in.flags			= ui::wf_visible;
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, theme.item_height};
		in.flow				= ui::flow_e::none;
		in.child_spacing	= 0.0f;
		in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		paint.set_rect(_root, rect);
		ui.set_pre_layout_tick(_root, on_pre_layout_tick, this);
	}

	void editor_tab_area_t::uninit()
	{
		_ui->cancel_mutations(this);
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
		_tabs.clear();
		_config				  = {};
		_active_tab			  = 0;
		_drag_tab			  = 0;
		_pending_close_tab	  = 0;
		_pending_drag_out_tab = 0;
		_drag_offset		  = {};
	}

	void editor_tab_area_t::update_markers(f32 dt)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (editor_tab_t& tab : _tabs)
		{
			const f32 target = tab.identifier == _active_tab ? 1.0f : 0.0f;
			if (dt <= 0.0f)
			{
				tab.marker_height	= target;
				tab.marker_velocity = 0.0f;
			}
			else
			{
				tab.marker_height = easing_t::smooth_damp(tab.marker_height, target, &tab.marker_velocity, 0.08f, 1000.0f, dt);
				tab.marker_height = math::clamp(tab.marker_height, 0.0f, 1.0f);
				if (math::abs(tab.marker_height - target) < 0.001f)
				{
					tab.marker_height	= target;
					tab.marker_velocity = 0.0f;
				}
			}

			ui::layout_in_t& marker_in = tree.in(tab.marker_inner);
			marker_in.size_value.y	   = tab.marker_height;
		}
	}

	void editor_tab_area_t::update_tab_positions(f32 dt)
	{
		ui::layout_tree_t&		tree	  = _ui->get_tree();
		const ui::layout_out_t& root	  = tree.out(_root);
		const vec2f_t&			mp		  = _ui->get_input().get_mouse_position();
		const f32				inv_scale = 1.0f / _ui->get_ui_scale();
		f32						slot_x	  = 0.0f;

		for (editor_tab_t& tab : _tabs)
		{
			const ui::layout_out_t& out		 = tree.out(tab.widget);
			f32						target_x = slot_x;
			f32						target_y = 0.0f;
			slot_x += out.size.x;

			const bool dragging = tab.identifier == _drag_tab;
			if (dragging)
			{
				target_x	   = mp.x - root.pos.x - _drag_offset.x;
				tab.pos_x	   = target_x;
				tab.pos_y	   = 0.0f;
				tab.velocity_x = 0.0f;
				tab.velocity_y = 0.0f;
			}
			else if (dt <= 0.0f)
			{
				tab.pos_x	   = target_x;
				tab.pos_y	   = target_y;
				tab.velocity_x = 0.0f;
				tab.velocity_y = 0.0f;
			}
			else
			{
				tab.pos_x = easing_t::smooth_damp(tab.pos_x, target_x, &tab.velocity_x, 0.08f, 1000.0f, dt);
				tab.pos_y = easing_t::smooth_damp(tab.pos_y, target_y, &tab.velocity_y, 0.08f, 1000.0f, dt);
			}

			ui::layout_in_t& in				  = tree.in(tab.widget);
			in.pos_value					  = {tab.pos_x * inv_scale, tab.pos_y * inv_scale};
			tree.draw_order(tab.widget)		  = dragging ? 1000u : 0u;
			tree.draw_order(tab.marker)		  = dragging ? 1000u : 0u;
			tree.draw_order(tab.marker_inner) = dragging ? 1000u : 0u;
			tree.draw_order(tab.label)		  = dragging ? 1000u : 0u;
			if (tab.close_button != NULL_WIDGET)
				tree.draw_order(tab.close_button) = dragging ? 1001u : 1u;
		}
	}

	void editor_tab_area_t::add_tab(const char* title)
	{
		SFG_ASSERT(title != nullptr);

		const sid_t identifier = TO_SID(title);
		for (const editor_tab_t& tab : _tabs)
			SFG_ASSERT(tab.identifier != identifier);

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t tab = ui.allocate_widget();
		ui.set_widget_debug_name(tab, "editor_tab");
		tree.attach(_root, tab);

		ui::layout_in_t& tab_in = tree.in(tab);
		tab_in.flags			= ui::wf_visible | ui::wf_input;
		tab_in.size_mode_x		= ui::axis_mode_e::sum_children;
		tab_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		tab_in.size_value		= {0.0f, 1.0f};
		tab_in.pos_mode_x		= ui::pos_mode_e::offset_in_parent;
		tab_in.pos_mode_y		= ui::pos_mode_e::offset_in_parent;
		tab_in.flow				= ui::flow_e::row;
		tab_in.child_spacing	= theme.item_spacing;
		tab_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		paint.set_custom(tab, draw_tab_frame, this);

		ui::listener_bundle_t tab_listener = {};
		tab_listener.user_data			   = this;
		tab_listener.on_click			   = on_tab_click;
		tab_listener.on_drag_begin		   = on_tab_drag_begin;
		tab_listener.on_drag			   = on_tab_drag;
		tab_listener.on_drag_end		   = on_tab_drag_end;
		ui.get_input().set_listener(tab, tab_listener);

		const ui::widget_id_t marker = ui.allocate_widget();
		ui.set_widget_debug_name(marker, "editor_tab_marker_wrapper");
		tree.attach(tab, marker);

		ui::layout_in_t& marker_in = tree.in(marker);
		marker_in.flags			   = ui::wf_visible;
		marker_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		marker_in.pos_value.y	   = 0.5f;
		marker_in.anchor_y		   = ui::anchor_e::center;
		marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
		marker_in.size_mode_y	   = ui::axis_mode_e::fixed;
		marker_in.size_value	   = {theme.item_height * 0.15f, theme.item_height * 0.65f};

		ui::vg_rect_paint_t marker_wrapper_rect = {};
		marker_wrapper_rect.fill_color_a		= theme.color_frame;
		marker_wrapper_rect.fill_color_b		= theme.color_frame;
		paint.set_rect(marker, marker_wrapper_rect);

		const ui::widget_id_t marker_inner = ui.allocate_widget();
		ui.set_widget_debug_name(marker_inner, "editor_tab_marker");
		tree.attach(marker, marker_inner);

		ui::layout_in_t& marker_inner_in = tree.in(marker_inner);
		marker_inner_in.flags			 = ui::wf_visible;
		marker_inner_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		marker_inner_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		marker_inner_in.pos_value		 = {0.0f, 1.0f};
		marker_inner_in.anchor_y		 = ui::anchor_e::end;
		marker_inner_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		marker_inner_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		marker_inner_in.size_value		 = {1.0f, _active_tab == 0 ? 1.0f : 0.0f};

		ui::vg_rect_paint_t marker_rect = {};
		marker_rect.fill_color_a		= theme.color_accent1_dim;
		marker_rect.fill_color_b		= theme.color_accent1;
		marker_rect.gradient			= ui::vg_gradient_e::vertical;
		paint.set_rect(marker_inner, marker_rect);

		const ui::widget_id_t label = ui.allocate_widget();
		ui.set_widget_debug_name(label, "editor_tab_label");
		tree.attach(tab, label);

		ui::layout_in_t& label_in = tree.in(label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(label, title);
		paint.set_text(
			label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		ui::widget_id_t close_button = NULL_WIDGET;
		if (_config.can_close)
		{
			close_button = editor_icon_widgets_t::add_naked_icon_button(ui, tab, ICON_CROSS, theme.icon_default_px_size, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text2);

			ui::listener_bundle_t close_listener = {};
			close_listener.user_data			 = this;
			close_listener.on_click				 = on_close_click;
			ui.get_input().set_listener(close_button, close_listener);
		}

		const f32 marker_height = _active_tab == 0 ? 1.0f : 0.0f;
		_tabs.push_back({.identifier = identifier, .widget = tab, .marker = marker, .marker_inner = marker_inner, .label = label, .close_button = close_button, .marker_height = marker_height});
		if (_active_tab == 0)
			_active_tab = identifier;
		update_markers(0.0f);
		update_tab_positions(0.0f);
		refresh_status();
	}

	void editor_tab_area_t::remove_tab(sid_t identifier)
	{
		remove_tab(identifier, true);
	}

	void editor_tab_area_t::remove_tab(sid_t identifier, bool notify_removed)
	{
		for (auto it = _tabs.begin(); it != _tabs.end(); ++it)
		{
			if (it->identifier == identifier)
			{
				const bool was_active = _active_tab == identifier;
				_ui->deallocate_widget(it->widget);
				_tabs.erase(it);
				if (was_active)
				{
					_active_tab = 0;
					if (!_tabs.empty())
						switch_tab(_tabs.front().identifier);
				}
				refresh_status();
				if (notify_removed && _config.tab_removed != nullptr)
					_config.tab_removed(*this, identifier, _config.user_data);
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_tab_area_t::select_tab(sid_t identifier)
	{
		switch_tab(identifier);
	}

	bool editor_tab_area_t::is_over_tab(const vec2f_t& pos) const
	{
		const ui::layout_tree_t& tree = _ui->get_tree();
		for (const editor_tab_t& tab : _tabs)
		{
			const ui::layout_out_t& out = tree.out(tab.widget);
			if (pos.x >= out.pos.x && pos.x <= out.pos.x + out.size.x && pos.y >= out.pos.y && pos.y <= out.pos.y + out.size.y)
				return true;
		}
		return false;
	}

	void editor_tab_area_t::refresh_status()
	{
		if (!_config.can_close)
			return;

		ui::layout_tree_t& tree = _ui->get_tree();
		for (const editor_tab_t& tab : _tabs)
		{
			const bool		 disabled = _config.close_allowed != nullptr ? !_config.close_allowed(*this, tab.identifier, _config.user_data) : !_config.can_close_single_tab && _tabs.size() <= 1;
			ui::layout_in_t& in		  = tree.in(tab.close_button);
			if (disabled)
				in.flags |= ui::wf_disabled;
			else
				in.flags &= ~ui::wf_disabled;
		}
	}

	void editor_tab_area_t::request_close(sid_t identifier)
	{
		_pending_close_tab = identifier;
		if (_drag_tab == identifier)
			_drag_tab = 0;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_tab_area_t::request_drag_out(sid_t identifier)
	{
		_pending_drag_out_tab = identifier;
		if (_drag_tab == identifier)
			_drag_tab = 0;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	bool editor_tab_area_t::consume_pending_removals()
	{
		if (_pending_drag_out_tab != 0)
		{
			const sid_t dragged	  = _pending_drag_out_tab;
			_pending_drag_out_tab = 0;
			remove_tab(dragged, false);
			if (_config.tab_dragged_out != nullptr)
				_config.tab_dragged_out(*this, dragged, _config.user_data);
			return true;
		}

		if (_pending_close_tab != 0)
		{
			const sid_t closed = _pending_close_tab;
			_pending_close_tab = 0;
			remove_tab(closed);
			return true;
		}

		return false;
	}

	void editor_tab_area_t::switch_tab(sid_t identifier)
	{
		_active_tab = identifier;
		if (_config.tab_switched != nullptr)
			_config.tab_switched(*this, identifier, _config.user_data);
	}

	size_t editor_tab_area_t::find_tab_index(sid_t identifier) const
	{
		for (size_t i = 0; i < _tabs.size(); ++i)
		{
			if (_tabs[i].identifier == identifier)
				return i;
		}

		SFG_ASSERT(false);
		return 0;
	}

	editor_tab_t& editor_tab_area_t::find_tab_by_widget(ui::widget_id_t widget)
	{
		for (editor_tab_t& tab : _tabs)
		{
			if (tab.widget == widget || tab.close_button == widget)
				return tab;
		}

		SFG_ASSERT(false);
		return _tabs.front();
	}

	void editor_tab_area_t::on_pre_layout_tick(ui::ui_context&, ui::widget_id_t, f32 dt, void* user_data)
	{
		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		tab_area.update_markers(dt);
		if (tab_area._drag_tab != 0 && tab_area.is_drag_out_position(tab_area._ui->get_input().get_mouse_position()))
			tab_area.request_drag_out(tab_area._drag_tab);

		tab_area.update_tab_positions(dt);
		if (tab_area._drag_tab != 0)
			tab_area.reorder_dragged_tab(tab_area._ui->get_input().get_mouse_position());
	}

	void editor_tab_area_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_tab_area_t*>(user_data)->consume_pending_removals();
	}

	void editor_tab_area_t::draw_tab_frame(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		editor_tab_area_t&		tab_area = *static_cast<editor_tab_area_t*>(user_data);
		const editor_theme_t&	theme	 = editor_theme_t::get();
		const ui::layout_out_t& out		 = tab_area._ui->get_tree().out(id);
		const f32				cut		 = out.size.y * 0.35f;

		const vec2f_t p0	  = {out.pos.x, out.pos.y};
		const vec2f_t p1	  = {out.pos.x + out.size.x - cut, out.pos.y};
		const vec2f_t p2	  = {out.pos.x + out.size.x, out.pos.y + cut};
		const vec2f_t p3	  = {out.pos.x + out.size.x, out.pos.y + out.size.y};
		const vec2f_t p4	  = {out.pos.x, out.pos.y + out.size.y};
		vec2f_t		  path[5] = {p0, p1, p2, p3, p4};

		const editor_tab_t&	  tab		 = tab_area.find_tab_by_widget(id);
		ui::layout_tree_t&	  tree		 = tab_area._ui->get_tree();
		const ui::widget_id_t hovered	 = tab_area._ui->get_input().get_hovered();
		bool				  is_hovered = hovered == id;
		ui::widget_id_t		  parent	 = hovered;
		while (!is_hovered && parent != NULL_WIDGET)
		{
			parent	   = tree.node(parent).parent;
			is_hovered = parent == id;
		}

		const vec4f_t color = tab.identifier == tab_area._active_tab ? theme.color_panel_light : (is_hovered ? theme.color_panel_light : theme.color_frame);

		ui::vg_convex_paint_t convex = {};
		convex.fill_color_a			 = color;
		convex.fill_color_b			 = color;
		convex.aa_thickness			 = theme.aa_thickness;

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;
		canvas.add_convex({path, 5}, convex, state, tab_area._ui->get_tree().draw_order_const(id));
	}

	void editor_tab_area_t::on_tab_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		editor_tab_t&	   tab		= tab_area.find_tab_by_widget(id);
		tab_area.switch_tab(tab.identifier);
	}

	void editor_tab_area_t::reorder_dragged_tab(const vec2f_t& pos)
	{
		if (_drag_tab == 0 || _tabs.size() < 2)
			return;

		const ui::layout_tree_t& tree	  = _ui->get_tree();
		const ui::layout_out_t&	 root_out = tree.out(_root);
		const f32				 local_x  = pos.x - root_out.pos.x;
		size_t					 target	  = 0;
		f32						 x		  = 0.0f;

		for (size_t i = 0; i < _tabs.size(); ++i)
		{
			if (_tabs[i].identifier == _drag_tab)
				continue;

			const f32 w = tree.out(_tabs[i].widget).size.x;
			if (local_x < x + w * 0.5f)
				break;

			x += w;
			target++;
		}

		const size_t current = find_tab_index(_drag_tab);
		if (target == current)
			return;

		editor_tab_t tab = _tabs[current];
		_tabs.erase(_tabs.begin() + current);
		_tabs.insert(_tabs.begin() + target, tab);
	}

	void editor_tab_area_t::finish_tab_drag(const vec2f_t& pos)
	{
		if (_drag_tab == 0)
			return;

		if (is_drag_out_position(pos))
			request_drag_out(_drag_tab);

		_drag_tab = 0;
	}

	bool editor_tab_area_t::is_drag_out_allowed(sid_t identifier)
	{
		if (!_config.can_drag_out)
			return false;

		if (_config.drag_out_allowed != nullptr)
			return _config.drag_out_allowed(*this, identifier, _config.user_data);

		return true;
	}

	bool editor_tab_area_t::is_drag_out_position(const vec2f_t& pos)
	{
		if (_drag_tab == 0 || !is_drag_out_allowed(_drag_tab))
			return false;

		const ui::layout_out_t& root_out = _ui->get_tree().out(_root);
		if (pos.y >= root_out.pos.y && pos.y <= root_out.pos.y + root_out.size.y)
			return false;

		return true;
	}

	void editor_tab_area_t::on_tab_drag_begin(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		editor_tab_t&	   tab		= tab_area.find_tab_by_widget(id);
		if (tab_area._tabs.size() < 2 && !tab_area.is_drag_out_allowed(tab.identifier))
		{
			tab_area.switch_tab(tab.identifier);
			return;
		}

		const ui::layout_out_t& out = tab_area._ui->get_tree().out(tab.widget);
		tab_area._drag_tab			= tab.identifier;
		tab_area._drag_offset		= pos - out.pos;
		tab.pos_x					= out.pos.x - tab_area._ui->get_tree().out(tab_area._root).pos.x;
		tab.pos_y					= out.pos.y - tab_area._ui->get_tree().out(tab_area._root).pos.y;
		tab_area.switch_tab(tab.identifier);
	}

	void editor_tab_area_t::on_tab_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		if (tab_area.is_drag_out_position(pos))
		{
			tab_area.request_drag_out(tab_area._drag_tab);
			return;
		}

		tab_area.reorder_dragged_tab(pos);
	}

	void editor_tab_area_t::on_tab_drag_end(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		tab_area.finish_tab_drag(pos);
	}

	void editor_tab_area_t::on_close_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		const editor_tab_t tab		= tab_area.find_tab_by_widget(id);
		tab_area.request_close(tab.identifier);
	}
}
