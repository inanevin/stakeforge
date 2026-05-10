// Copyright (c) 2025 Inan Evin

#include "widgets/editor_dividers.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/math/color.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	ui::widget_id_t editor_dividers_t::make_divider_horizontal(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, theme.divider_thickness};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = color;
		rect.fill_color_b		 = color;
		paint.set_rect(id, rect);
		return id;
	}

	ui::widget_id_t editor_dividers_t::make_divider_vertical(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {theme.divider_thickness, 1.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = color;
		rect.fill_color_b		 = color;
		paint.set_rect(id, rect);
		return id;
	}

	ui::widget_id_t editor_dividers_t::make_divider_horizontal_dropshadow(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color, bool flip)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, theme.divider_thickness * 4.0f};

		const vec4f_t color_a = color;
		const vec4f_t color_b = {color.x, color.y, color.z, 0.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = flip ? color_b : color_a;
		rect.fill_color_b		 = flip ? color_a : color_b;
		rect.gradient			 = ui::vg_gradient_e::vertical;
		paint.set_rect(id, rect);
		return id;
	}

	ui::widget_id_t editor_dividers_t::make_divider_vertical_dropshadow(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& color, bool flip)
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {theme.divider_thickness * 2.0f, 1.0f};

		const vec4f_t color_a = color;
		const vec4f_t color_b = {color.x, color.y, color.z, 0.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = flip ? color_b : color_a;
		rect.fill_color_b		 = flip ? color_a : color_b;
		rect.gradient			 = ui::vg_gradient_e::horizontal;
		paint.set_rect(id, rect);
		return id;
	}
}
