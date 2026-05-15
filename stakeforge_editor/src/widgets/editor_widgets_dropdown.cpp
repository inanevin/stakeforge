// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_dropdown.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <cmath>

namespace sfg
{
	namespace
	{
		constexpr u32 DROPDOWN_DRAW_ORDER	   = 49000u;
		constexpr u32 SELECTED_MARKER_SEGMENTS = 16;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u16>(ui::wf_visible | (input ? ui::wf_input : 0)) : static_cast<u16>(ui::wf_overlay);
		}
	}

	void editor_dropdown_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_dropdown_config_t& config)
	{
		SFG_ASSERT(config.item_count <= MAX_ITEMS);
		SFG_ASSERT(config.items != nullptr);

		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "dropdown");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::sum_children;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.pos_mode_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::pos_mode_e::relative_in_parent : ui::pos_mode_e::flow;
		root_in.pos_value.y		 = config.pos_y == editor_dropdown_pos_y_e::center ? 0.5f : 0.0f;
		root_in.anchor_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::anchor_e::center : ui::anchor_e::start;
		root_in.flow			 = config.width == editor_dropdown_width_e::sum_children ? ui::flow_e::row : ui::flow_e::none;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		if (config.width == editor_dropdown_width_e::parent_relative)
			root_in.size_mode_x = ui::axis_mode_e::parent_relative;
		else if (config.width == editor_dropdown_width_e::fixed)
		{
			root_in.size_mode_x	 = ui::axis_mode_e::fixed;
			root_in.size_value.x = config.fixed_width;
		}

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.rounding			 = theme.item_rounding;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.outline_color		 = theme.color_panel_light;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);

		ui::listener_bundle_t root_listener = {};
		root_listener.user_data				= this;
		root_listener.on_click				= on_root_click;
		ui.get_input().set_listener(_root, root_listener);

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "dropdown_title");
		tree.attach(_root, _title);
		tree.draw_order(_title) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& title_in = tree.in(_title);
		title_in.size_mode_x	  = ui::axis_mode_e::fixed;
		title_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		title_in.size_value		  = {0.0f, 1.0f};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			title_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			title_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value	= {0.0f, 0.5f};
			title_in.anchor_y	= ui::anchor_e::center;
		}
		else
		{
			title_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value.y = 0.5f;
			title_in.anchor_y	 = ui::anchor_e::center;
		}

		paint.set_text(_title, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_icon_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_icon_frame, "dropdown_icon");
		tree.attach(_root, _icon_frame);
		tree.draw_order(_icon_frame) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& icon_in = tree.in(_icon_frame);
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			icon_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value  = {1.0f, 0.5f};
			icon_in.anchor_x   = ui::anchor_e::end;
			icon_in.anchor_y   = ui::anchor_e::center;
		}
		else
		{
			icon_in.pos_mode_y	= ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value.y = 0.5f;
			icon_in.anchor_y	= ui::anchor_e::center;
		}
		editor_icon_widgets_t::add_icon(ui, _icon_frame, ICON_DD_DOWN, theme.icon_default_px_size, theme.color_text1);

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "dropdown_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = DROPDOWN_DRAW_ORDER;

		ui::layout_in_t& fg_in = tree.in(_foreground);
		fg_in.flags			   = ui::wf_overlay;
		fg_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		fg_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		fg_in.size_value	   = {1.0f, 1.0f};

		_panel = ui.allocate_widget();
		ui.set_widget_debug_name(_panel, "dropdown_panel");
		tree.attach(_foreground, _panel);
		tree.draw_order(_panel) = DROPDOWN_DRAW_ORDER + 1u;

		ui::layout_in_t& panel_in = tree.in(_panel);
		panel_in.flags			  = ui::wf_overlay;
		panel_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
		panel_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
		panel_in.size_mode_x	  = ui::axis_mode_e::fixed;
		panel_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		panel_in.flow			  = ui::flow_e::column;
		panel_in.child_spacing	  = 0.0f;
		panel_in.child_margins	  = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t panel_rect = {};
		panel_rect.fill_color_a		   = theme.color_frame;
		panel_rect.fill_color_b		   = theme.color_frame;
		panel_rect.outline_color	   = theme.color_outline;
		panel_rect.outline_thickness   = theme.outline_thickness;
		paint.set_rect(_panel, panel_rect);

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = on_row_click;

		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			const ui::widget_id_t row = ui.allocate_widget();
			_row_frames[i]			  = row;
			ui.set_widget_debug_name(row, "dropdown_item");
			tree.attach(_panel, row);
			tree.draw_order(row) = tree.draw_order_const(_panel) + 1;

			ui::layout_in_t& row_in = tree.in(row);
			row_in.flags			= ui::wf_overlay;
			row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			row_in.size_mode_y		= ui::axis_mode_e::fixed;
			row_in.size_value		= {1.0f, theme.item_height};
			row_in.flow				= ui::flow_e::row;
			row_in.child_spacing	= 0.0f;
			row_in.child_margins	= {0.0f, theme.indent_horizontal, 0.0f, 0.0f};

			ui::vg_rect_paint_t row_rect = {};
			row_rect.fill_color_a		 = {0, 0, 0, 0};
			row_rect.fill_color_b		 = {0, 0, 0, 0};
			paint.set_rect(row, row_rect);
			paint.set_hover_color(row, theme.color_panel);
			paint.set_press_color(row, theme.color_frame_light);
			ui.get_input().set_listener(row, row_listener);

			const ui::widget_id_t marker = ui.allocate_widget();
			_row_markers[i]				 = marker;
			ui.set_widget_debug_name(marker, "dropdown_selected_marker");
			tree.attach(row, marker);
			tree.draw_order(marker) = tree.draw_order_const(row) + 1;

			ui::layout_in_t& marker_in = tree.in(marker);
			marker_in.flags			   = ui::wf_overlay;
			marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
			marker_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			marker_in.size_value	   = {theme.item_height, 1.0f};
			paint.set_custom(marker, draw_selected_marker, this);

			const ui::widget_id_t label = ui.allocate_widget();
			_row_labels[i]				= label;
			ui.set_widget_debug_name(label, "dropdown_item_label");
			tree.attach(row, label);
			tree.draw_order(label) = tree.draw_order_const(row) + 1;

			ui::layout_in_t& label_in = tree.in(label);
			label_in.flags			  = ui::wf_overlay;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;
			paint.set_text(label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		refresh_title();
	}

	void editor_dropdown_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui->deallocate_widget(_foreground);

		_ui			= nullptr;
		_root		= NULL_WIDGET;
		_title		= NULL_WIDGET;
		_icon_frame = NULL_WIDGET;
		_foreground = NULL_WIDGET;
		_panel		= NULL_WIDGET;
		_config		= {};
		_open		= false;
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			_row_frames[i]	= NULL_WIDGET;
			_row_markers[i] = NULL_WIDGET;
			_row_labels[i]	= NULL_WIDGET;
		}
	}

	void editor_dropdown_t::close()
	{
		if (!_open)
			return;

		_open = false;
		set_popup_visible(false);
		_ui->get_input().clear_popup_scope();
	}

	void editor_dropdown_t::refresh_title()
	{
		SFG_ASSERT(_ui != nullptr);
		_ui->set_widget_text(_title, get_selected_text());
		ui::layout_in_t& title_in = _ui->get_tree().in(_title);
		title_in.size_value.x	  = static_cast<f32>(_ui->widget_text_len(_title)) * editor_theme_t::get().text_default_px_size * 0.7f;
		_ui->get_paint().set_text(
			_title,
			_ui->widget_text(_title),
			_ui->widget_text_len(_title),
			{.font = editor_theme_t::get().font_default, .color = editor_theme_t::get().color_text0, .point_size = editor_theme_t::get().text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	u16 editor_dropdown_t::get_selected() const
	{
		return _config.selected != nullptr ? _config.selected(_config.user_data) : (_config.item_count > 0 ? _config.items[0].value : 0);
	}

	const char* editor_dropdown_t::get_selected_text() const
	{
		if (!_config.title_from_selection && _config.title != nullptr)
			return _config.title;

		const u16 selected = get_selected();
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			if (_config.items[i].value == selected)
				return _config.items[i].text;
		}
		return _config.item_count > 0 ? _config.items[0].text : "";
	}

	void editor_dropdown_t::open()
	{
		_open = true;
		refresh_title();
		refresh_rows();
		set_popup_visible(true);

		ui::widget_id_t popup_roots[] = {_panel};
		_ui->get_input().set_popup_scope(_root, popup_roots, 1, on_popup_outside, this);
	}

	void editor_dropdown_t::set_popup_visible(bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, _foreground, visible, false);
		set_widget_visible(tree, _panel, visible, false);
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			const bool show = visible && i < _config.item_count;
			set_widget_visible(tree, _row_frames[i], show, show);
			set_widget_visible(tree, _row_markers[i], show, false);
			set_widget_visible(tree, _row_labels[i], show, false);
		}
	}

	void editor_dropdown_t::refresh_rows()
	{
		ui::layout_tree_t&		tree	 = _ui->get_tree();
		ui::paint_layer_t&		paint	 = _ui->get_paint();
		const editor_theme_t&	theme	 = editor_theme_t::get();
		const ui::layout_out_t& root_out = tree.out(_root);
		const ui::layout_out_t& screen	 = tree.out(tree.get_root());
		f32						width	 = root_out.size.x;
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			_ui->set_widget_text(_row_labels[i], _config.items[i].text);
			paint.set_text(_row_labels[i],
						   _ui->widget_text(_row_labels[i]),
						   _ui->widget_text_len(_row_labels[i]),
						   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			width = math::max(width, theme.item_height + static_cast<f32>(_ui->widget_text_len(_row_labels[i])) * theme.text_default_px_size * 0.7f + theme.indent_horizontal * 2.0f);
		}

		const f32 scale		= _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32 width_px	= width * scale;
		const f32 height_px = (static_cast<f32>(_config.item_count) * theme.item_height + theme.margin_vertical * 2.0f) * scale;
		f32		  x			= root_out.pos.x;
		f32		  y			= root_out.pos.y + root_out.size.y + theme.item_spacing;
		if (x + width_px > screen.clip.x + screen.clip.z)
			x = screen.clip.x + screen.clip.z - width_px;
		if (y + height_px > screen.clip.y + screen.clip.w)
			y = root_out.pos.y - height_px;
		x = math::clamp(x, screen.clip.x, math::max(screen.clip.x, screen.clip.x + screen.clip.z - width_px));
		y = math::clamp(y, screen.clip.y, math::max(screen.clip.y, screen.clip.y + screen.clip.w - height_px));

		ui::layout_in_t& panel_in = tree.in(_panel);
		panel_in.pos_value		  = {x, y};
		panel_in.size_value		  = {width, 0.0f};
	}

	u32 editor_dropdown_t::find_row(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			if (_row_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	void editor_dropdown_t::on_root_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(user_data);
		if (dropdown._open)
			dropdown.close();
		else
			dropdown.open();
	}

	void editor_dropdown_t::on_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(user_data);
		const u32		   row		= dropdown.find_row(id);
		const u16		   value	= dropdown._config.items[row].value;
		if (dropdown._config.pressed != nullptr)
			dropdown._config.pressed(value, dropdown._config.user_data);
		dropdown.refresh_title();
		dropdown.close();
	}

	void editor_dropdown_t::on_popup_outside(ui::input_router_t&, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn == ui::mouse_button_e::left)
			static_cast<editor_dropdown_t*>(user_data)->close();
	}

	void editor_dropdown_t::draw_selected_marker(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(user_data);
		u32				   row		= dropdown.find_row(dropdown._ui->get_tree().node(id).parent);
		if (dropdown._config.items[row].value != dropdown.get_selected())
			return;

		const editor_theme_t&	theme						   = editor_theme_t::get();
		const ui::layout_out_t& out							   = dropdown._ui->get_tree().out(id);
		const f32				radius						   = math::max(2.0f, math::min(out.size.x, out.size.y) * 0.18f);
		const vec2f_t			center						   = {out.pos.x + out.size.x * 0.5f, out.pos.y + out.size.y * 0.5f};
		vec2f_t					path[SELECTED_MARKER_SEGMENTS] = {};
		for (u32 i = 0; i < SELECTED_MARKER_SEGMENTS; ++i)
		{
			const f32 a = static_cast<f32>(i) / static_cast<f32>(SELECTED_MARKER_SEGMENTS) * 6.28318530718f;
			path[i]		= {center.x + std::cos(a) * radius, center.y + std::sin(a) * radius};
		}

		ui::ui_render_state_t state	 = {};
		state.pipeline				 = paint.get_pipelines().default_pipeline;
		ui::vg_convex_paint_t marker = {};
		marker.fill_color_a			 = theme.color_accent0_light;
		marker.fill_color_b			 = theme.color_accent0;
		marker.gradient				 = ui::vg_gradient_e::vertical;
		marker.aa_thickness			 = theme.aa_thickness;
		canvas.add_convex({path, SELECTED_MARKER_SEGMENTS}, marker, state, dropdown._ui->get_tree().draw_order_const(id));
	}
}
