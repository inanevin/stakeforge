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
#include "ui/editor_modal_assets_override.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_modal_assets_override_t::set_rows(const char* const* rows, u16 row_count)
	{
		_rows.resize(0);
		_rows.reserve(row_count);
		for (u16 i = 0; i < row_count; ++i)
			_rows.push_back(rows[i] != nullptr ? rows[i] : "");
	}

	void editor_modal_assets_override_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "modal_assets_override");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::max_children;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing * 0.5f;

		_row_widgets.resize(0);
		_row_widgets.reserve(_rows.size());
		for (const string_t& row : _rows)
		{
			const ui::widget_id_t label = ui.allocate_widget();
			ui.set_widget_debug_name(label, "modal_assets_override_row");
			tree.attach(_root, label);

			ui::layout_in_t& label_in = tree.in(label);
			label_in.flags			  = ui::wf_visible;
			ui.set_widget_text(label, row.c_str());
			paint.set_text(
				label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default_mono, .color = theme.color_text1, .point_size = theme.text_small_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			_row_widgets.push_back(label);
		}
	}

	void editor_modal_assets_override_t::uninit()
	{
		_ui->deallocate_widget(_root);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
		_row_widgets.resize(0);
	}

	editor_modal_content_desc_t editor_modal_assets_override_t::get_content_desc()
	{
		return {.init = init_content, .uninit = uninit_content, .user_data = this};
	}

	void editor_modal_assets_override_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_assets_override_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_assets_override_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_assets_override_t*>(user_data)->uninit();
	}
}
