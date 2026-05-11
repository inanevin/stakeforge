// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_dividers.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	ui::widget_id_t editor_dividers_t::add_divider_hor(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, thickness};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = color_a;
		rect.fill_color_b		 = color_b;
		rect.gradient			 = gradient;
		paint.set_rect(id, rect);
		return id;
	}

	ui::widget_id_t editor_dividers_t::add_divider_ver(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= tree.allocate();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {thickness, 1.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = color_a;
		rect.fill_color_b		 = color_b;
		rect.gradient			 = gradient;
		paint.set_rect(id, rect);
		return id;
	}
}
