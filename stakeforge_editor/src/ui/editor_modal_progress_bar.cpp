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
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <cstdio>

namespace sfg
{
	void editor_modal_progress_bar_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "modal_progress_bar");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {theme.item_width * 3.0f, theme.item_height};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.outline_color		 = theme.color_outline_light;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		rect.aa_thickness		 = theme.aa_thickness;
		paint.set_rect(_root, rect);

		_fill = ui.allocate_widget();
		ui.set_widget_debug_name(_fill, "modal_progress_bar_fill");
		tree.attach(_root, _fill);
		tree.draw_order(_fill) = tree.draw_order_const(_root);

		ui::layout_in_t& fill_in = tree.in(_fill);
		fill_in.flags			 = ui::wf_visible;
		fill_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		fill_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		fill_in.size_value		 = {_progress, 1.0f};

		ui::vg_rect_paint_t fill_rect = {};
		fill_rect.fill_color_a		  = theme.color_accent1_dim;
		fill_rect.fill_color_b		  = theme.color_accent1;
		fill_rect.gradient			  = ui::vg_gradient_e::horizontal;
		fill_rect.rounding			  = theme.item_rounding;
		fill_rect.rounding_segs		  = 4;
		fill_rect.aa_thickness		  = theme.aa_thickness;
		paint.set_rect(_fill, fill_rect);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "modal_progress_bar_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;
		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		refresh();
	}

	void editor_modal_progress_bar_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui	   = nullptr;
		_root  = NULL_WIDGET;
		_fill  = NULL_WIDGET;
		_label = NULL_WIDGET;
	}

	void editor_modal_progress_bar_t::set_progress(f32 progress)
	{
		_progress = math::clamp(progress, 0.0f, 1.0f);
		if (_ui != nullptr)
			refresh();
	}

	editor_modal_content_desc_t editor_modal_progress_bar_t::get_content_desc()
	{
		return {.init = init_content, .uninit = uninit_content, .user_data = this};
	}

	void editor_modal_progress_bar_t::refresh()
	{
		_ui->get_tree().in(_fill).size_value.x = _progress;

		char text[8] = {};
		snprintf(text, sizeof(text), "%u%%", static_cast<u32>(math::round(_progress * 100.0f)));
		_ui->set_widget_text(_label, text);
	}

	void editor_modal_progress_bar_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_progress_bar_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_progress_bar_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_progress_bar_t*>(user_data)->uninit();
	}
}
