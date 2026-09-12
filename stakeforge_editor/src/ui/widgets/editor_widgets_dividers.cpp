/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "ui/widgets/editor_widgets_dividers.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	ui::widget_id_t editor_dividers_t::add_divider_hor(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= ui.allocate_widget();
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
		const ui::widget_id_t id	= ui.allocate_widget();
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
	ui::widget_id_t editor_dividers_t::add_divider_ver(ui::ui_context& ui, ui::widget_id_t parent, f32 thickness, f32 height, const vec4f_t& color_a, const vec4f_t& color_b, ui::vg_gradient_e gradient)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const ui::widget_id_t id	= ui.allocate_widget();
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value.y		= 0.5f;
		in.anchor_y			= ui::anchor_e::center;
		in.size_value		= {thickness, height};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = color_a;
		rect.fill_color_b		 = color_b;
		rect.gradient			 = gradient;
		paint.set_rect(id, rect);
		return id;
	}
}
