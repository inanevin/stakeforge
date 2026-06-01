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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ui/widgets/editor_widget_spinner.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/math/math.hpp>
#include <sfg/math/math_common.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	namespace
	{
#define SPINNER_SEGMENTS 45
	}

	void editor_widget_spinner_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_spinner_config_t& config)
	{
		_ui			 = &ui;
		_outer_color = config.outer_color;
		_inner_color = config.inner_color;

		ui::layout_tree_t& tree	 = ui.get_tree();
		ui::paint_layer_t& paint = ui.get_paint();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "spinner");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};

		paint.set_custom(_root, draw, this);
	}

	void editor_widget_spinner_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui			 = nullptr;
		_root		 = NULL_WIDGET;
		_outer_color = {};
		_inner_color = {};
	}

	void editor_widget_spinner_t::draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		editor_widget_spinner_t& spinner = *static_cast<editor_widget_spinner_t*>(user_data);
		const ui::layout_out_t&	 out	 = spinner._ui->get_tree().out(id);
		const f32				 size	 = math::min(out.size.x, out.size.y);
		if (size <= 0.0f)
			return;

		const vec2f_t center	 = {out.pos.x + out.size.x * 0.5f, out.pos.y + out.size.y * 0.5f};
		const f32	  radius	 = size * 0.42f;
		const f32	  thickness	 = 1.0f;
		const u32	  draw_order = spinner._ui->get_tree().draw_order_const(id);

		static f32 seconds = 0.0f;
		seconds += (1.0f / 60.0f) * 0.25f;

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;

		const editor_theme_t& theme	 = editor_theme_t::get();
		const f32			  start0 = seconds;
		const f32			  start1 = -seconds;
		const f32			  end0	 = start0 + MATH_TWO_PI * 0.75f;
		const f32			  end1	 = start1 + MATH_TWO_PI * 0.75f;

		const ui::vg_arc_paint_t outer_paint{.color = spinner._outer_color, .thickness = thickness, .aa_thickness = theme.aa_thickness, .segments = SPINNER_SEGMENTS};
		const ui::vg_arc_paint_t inner_paint{.color = spinner._inner_color, .thickness = thickness, .aa_thickness = theme.aa_thickness, .segments = SPINNER_SEGMENTS};
		canvas.add_arc(center, radius, start0, end0, outer_paint, state, draw_order);
		canvas.add_arc(center, radius * 0.75f, start1, end1, inner_paint, state, draw_order);
	}
}
