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
#include "ui/widgets/editor_widget_progress_bar.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/data/char_util.hpp>
namespace sfg
{
	void editor_widget_progress_bar_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_progress_bar_config_t& config)
	{
		_ui							= &ui;
		_progress_amount			= math::clamp(config.progress_amount, 0.0f, 1.0f);
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "progress_bar");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = 1.0f;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing;

		_progress_text = ui.allocate_widget();
		ui.set_widget_debug_name(_progress_text, "progress_text");
		tree.attach(_root, _progress_text);

		ui::layout_in_t& text_in = tree.in(_progress_text);

		ui.set_widget_text(_progress_text, config.progress_text);
		paint.set_text(_progress_text,
					   ui.widget_text(_progress_text),
					   ui.widget_text_len(_progress_text),
					   {.font = theme.font_title, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_progress_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_progress_frame, "progress_frame");
		tree.attach(_root, _progress_frame);

		ui::layout_in_t& frame_in = tree.in(_progress_frame);
		frame_in.pos_mode_y		  = ui::pos_mode_e::flow;
		frame_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		frame_in.size_mode_y	  = ui::axis_mode_e::fixed;
		frame_in.size_value		  = {1.0f, config.frame_height};
		frame_in.child_margins	  = {2.0f, 2.0f, 2.0f, 2.0f};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame;
		frame_rect.fill_color_b		   = theme.color_frame;
		frame_rect.outline_color	   = theme.color_outline_light;
		frame_rect.outline_thickness   = theme.outline_thickness;
		frame_rect.rounding			   = theme.item_rounding;
		frame_rect.rounding_segs	   = 4;
		frame_rect.aa_thickness		   = theme.aa_thickness;
		paint.set_rect(_progress_frame, frame_rect);

		_progress_fill = ui.allocate_widget();
		ui.set_widget_debug_name(_progress_fill, "progress_fill");
		tree.attach(_progress_frame, _progress_fill);
		tree.draw_order(_progress_fill) = tree.draw_order_const(_progress_frame) + 1;

		ui::layout_in_t& fill_in = tree.in(_progress_fill);
		fill_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		fill_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		fill_in.size_value		 = {_progress_amount, 1.0f};

		ui::vg_rect_paint_t fill_rect = {};
		fill_rect.fill_color_a		  = theme.color_accent1_dim;
		fill_rect.fill_color_b		  = theme.color_accent1;
		fill_rect.gradient			  = ui::vg_gradient_e::horizontal;
		fill_rect.rounding			  = theme.item_rounding;
		fill_rect.rounding_segs		  = 4;
		fill_rect.aa_thickness		  = theme.aa_thickness;
		paint.set_rect(_progress_fill, fill_rect);

		_progress_amount_label = ui.allocate_widget();
		ui.set_widget_debug_name(_progress_amount_label, "progress_amount_label");
		tree.attach(_progress_frame, _progress_amount_label);
		tree.draw_order(_progress_amount_label) = tree.draw_order_const(_progress_fill);

		ui::layout_in_t& amount_in = tree.in(_progress_amount_label);
		amount_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		amount_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		amount_in.pos_value		   = {0.5f, 0.5f};
		amount_in.anchor_x		   = ui::anchor_e::center;
		amount_in.anchor_y		   = ui::anchor_e::center;
		paint.set_text(_progress_amount_label, nullptr, 0, {.font = theme.font_title, .color = theme.color_accent0_light, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		refresh_progress_amount();
	}

	void editor_widget_progress_bar_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui					   = nullptr;
		_root				   = NULL_WIDGET;
		_progress_text		   = NULL_WIDGET;
		_progress_frame		   = NULL_WIDGET;
		_progress_fill		   = NULL_WIDGET;
		_progress_amount_label = NULL_WIDGET;
		_progress_amount	   = 0.0f;
	}

	void editor_widget_progress_bar_t::update_progress(f32 progress)
	{
		_progress_amount = math::clamp(progress, 0.0f, 1.0f);
		refresh_progress_amount();
	}

	void editor_widget_progress_bar_t::update_progress_text(const char* text)
	{
		_ui->set_widget_text(_progress_text, text);
	}

	void editor_widget_progress_bar_t::refresh_progress_amount()
	{
		_ui->get_tree().in(_progress_fill).size_value.x = _progress_amount;

		const u32 amt	   = _progress_amount * 100.0f;
		char	  text[16] = {};
		char*	  cur	   = text;
		char*	  end	   = cur + sizeof(text);
		char_util::append(cur, end, "%");
		char_util::append_u32(cur, end, amt);

		_ui->set_widget_text(_progress_amount_label, text);
	}
}
