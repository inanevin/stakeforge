// Copyright (c) 2025 Inan Evin

#include "widgets/editor_tab_area.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
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
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, theme.item_height};
		in.flow				= ui::flow_e::row;
		in.child_spacing	= 0.0f;
		in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_bg0;
		rect.fill_color_b		 = theme.color_bg0;
		paint.set_rect(_root, rect);
	}

	void editor_tab_area_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
		_tabs.clear();
		_config		= {};
		_active_tab = 0;
	}

	void editor_tab_area_t::update(f32 dt)
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
		tab_in.flow				= ui::flow_e::row;
		tab_in.child_spacing	= theme.item_spacing;
		tab_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		paint.set_custom(tab, draw_tab_frame, this);

		ui::listener_bundle_t tab_listener = {};
		tab_listener.user_data			   = this;
		tab_listener.on_click			   = on_tab_click;
		ui.get_input().set_listener(tab, tab_listener);

		const ui::widget_id_t marker = ui.allocate_widget();
		ui.set_widget_debug_name(marker, "editor_tab_marker_wrapper");
		tree.attach(tab, marker);

		ui::layout_in_t& marker_in = tree.in(marker);
		marker_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		marker_in.pos_value.y	   = 0.5f;
		marker_in.anchor_y		   = ui::anchor_e::center;
		marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
		marker_in.size_mode_y	   = ui::axis_mode_e::fixed;
		marker_in.size_value	   = {theme.item_height * 0.15f, theme.item_height * 0.65f};

		ui::vg_rect_paint_t marker_wrapper_rect = {};
		marker_wrapper_rect.fill_color_a		= theme.color_bg0;
		marker_wrapper_rect.fill_color_b		= theme.color_bg0;
		paint.set_rect(marker, marker_wrapper_rect);

		const ui::widget_id_t marker_inner = ui.allocate_widget();
		ui.set_widget_debug_name(marker_inner, "editor_tab_marker");
		tree.attach(marker, marker_inner);

		ui::layout_in_t& marker_inner_in = tree.in(marker_inner);
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
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(label, title);
		paint.set_text(
			label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_fg4, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		ui::widget_id_t close_button = NULL_WIDGET;
		if (_config.can_close)
		{
			close_button = editor_icon_widgets_t::add_naked_icon_button(ui, tab, ICON_CROSS, theme.icon_default_px_size, theme.color_fg1, theme.color_accent1, theme.color_accent1_dim, theme.color_fg0);

			ui::listener_bundle_t close_listener = {};
			close_listener.user_data			 = this;
			close_listener.on_click				 = on_close_click;
			ui.get_input().set_listener(close_button, close_listener);
		}

		const f32 marker_height = _active_tab == 0 ? 1.0f : 0.0f;
		_tabs.push_back({.identifier = identifier, .widget = tab, .marker = marker, .marker_inner = marker_inner, .close_button = close_button, .marker_height = marker_height});
		if (_active_tab == 0)
			_active_tab = identifier;
		update(0.0f);
		refresh_status();
	}

	void editor_tab_area_t::remove_tab(sid_t identifier)
	{
		for (auto it = _tabs.begin(); it != _tabs.end(); ++it)
		{
			if (it->identifier == identifier)
			{
				const bool was_active = _active_tab == identifier;
				_ui->deallocate_widget(it->widget);
				_tabs.erase(it);
				if (_config.tab_removed != nullptr)
					_config.tab_removed(*this, identifier, _config.user_data);
				if (was_active)
				{
					_active_tab = 0;
					if (!_tabs.empty())
						switch_tab(_tabs.front().identifier);
				}
				refresh_status();
				return;
			}
		}

		SFG_ASSERT(false);
	}

	void editor_tab_area_t::select_tab(sid_t identifier)
	{
		switch_tab(identifier);
	}

	void editor_tab_area_t::refresh_status()
	{
		if (!_config.can_close)
			return;

		const bool		   disabled = !_config.can_close_single_tab && _tabs.size() <= 1;
		ui::layout_tree_t& tree		= _ui->get_tree();
		for (const editor_tab_t& tab : _tabs)
		{
			ui::layout_in_t& in = tree.in(tab.close_button);
			if (disabled)
				in.flags |= ui::wf_disabled;
			else
				in.flags &= ~ui::wf_disabled;
		}
	}

	void editor_tab_area_t::switch_tab(sid_t identifier)
	{
		_active_tab = identifier;
		if (_config.tab_switched != nullptr)
			_config.tab_switched(*this, identifier, _config.user_data);
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

		const vec4f_t color = tab.identifier == tab_area._active_tab ? theme.color_bg4 : (is_hovered ? theme.color_bg4 : theme.color_bg2);

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

	void editor_tab_area_t::on_close_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_tab_area_t& tab_area = *static_cast<editor_tab_area_t*>(user_data);
		const editor_tab_t tab		= tab_area.find_tab_by_widget(id);
		tab_area.remove_tab(tab.identifier);
	}
}
