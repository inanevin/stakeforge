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
#include "ui/widgets/editor_widgets_button.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void style_button(ui::paint_layer_t& paint, ui::widget_id_t id)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui::vg_rect_paint_t	  rect	= {};
			rect.fill_color_a			= theme.color_panel_light;
			rect.fill_color_b			= theme.color_panel_light;
			rect.outline_color			= theme.color_outline_light;
			rect.outline_thickness		= theme.outline_thickness;
			rect.rounding				= theme.item_rounding;
			rect.rounding_segs			= 4;
			paint.set_rect(id, rect);
			paint.set_hover_color(id, theme.color_panel);
			paint.set_press_color(id, theme.color_frame_light);
			paint.set_focus_color(id, theme.color_accent0);
		}

		void set_label_text(ui::ui_context& ui, ui::widget_id_t id, const char* text)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui.set_widget_text(id, text != nullptr ? text : "");
			ui.get_paint().set_text(
				id, ui.widget_text(id), ui.widget_text_len(id), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}
	}

	void editor_button_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_button_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "button");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		apply_editor_widget_width(root_in, config.width);
		root_in.size_mode_y	 = ui::axis_mode_e::fixed;
		root_in.size_value.y = theme.item_height;
		style_button(paint, _root);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "button_label");
		tree.attach(_root, _label);
		tree.draw_order(_label) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;
		set_label_text(ui, _label, config.text);
	}

	void editor_button_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_label	= NULL_WIDGET;
		_config = {};
	}

	void editor_button_t::set_text(const char* text)
	{
		_config.text = text;
		set_label_text(*_ui, _label, text);
	}
}
