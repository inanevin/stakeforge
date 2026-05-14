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
}
