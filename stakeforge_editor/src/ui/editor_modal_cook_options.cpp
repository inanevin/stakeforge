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
#include "ui/editor_modal_cook_options.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_modal_cook_options_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "modal_cook_options");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;

		ui.set_widget_text(_root, "Cook options");
		paint.set_text(
			_root, ui.widget_text(_root), ui.widget_text_len(_root), {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_modal_cook_options_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	editor_modal_content_desc_t editor_modal_cook_options_t::get_content_desc()
	{
		return {.init = init_content, .uninit = uninit_content, .user_data = this};
	}

	void editor_modal_cook_options_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_cook_options_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_cook_options_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_cook_options_t*>(user_data)->uninit();
	}
}
