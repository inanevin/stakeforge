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
#include "ui/panels/editor_theme.hpp"

#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define MODAL_PROGRESS_BAR_FRAME_WIDTH_X 0.3f

	void editor_modal_progress_bar_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "modal_progress_content");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = 1.0f;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing;

		_bar.init(ui, _root, {.progress_text = "", .progress_amount = _progress, .frame_height = theme.item_height});

		_spinner_row = ui.allocate_widget();
		ui.set_widget_debug_name(_spinner_row, "modal_progress_spinner_row");
		tree.attach(_root, _spinner_row);

		ui::layout_in_t& spinner_row_in = tree.in(_spinner_row);
		spinner_row_in.flags			= ui::wf_visible;
		spinner_row_in.pos_mode_y		= ui::pos_mode_e::flow;
		spinner_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		spinner_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		spinner_row_in.size_value		= {1.0f, theme.item_height * 2.0f};

		_spinner_holder = ui.allocate_widget();
		ui.set_widget_debug_name(_spinner_holder, "modal_progress_spinner_holder");
		tree.attach(_spinner_row, _spinner_holder);

		ui::layout_in_t& spinner_holder_in = tree.in(_spinner_holder);
		spinner_holder_in.flags			   = ui::wf_visible;
		spinner_holder_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		spinner_holder_in.pos_mode_y	   = ui::pos_mode_e::flow;
		spinner_holder_in.pos_value.x	   = 0.5f;
		spinner_holder_in.anchor_x		   = ui::anchor_e::center;
		spinner_holder_in.size_mode_x	   = ui::axis_mode_e::fixed;
		spinner_holder_in.size_mode_y	   = ui::axis_mode_e::fixed;
		spinner_holder_in.size_value	   = {theme.item_height * 2.0f, theme.item_height * 2.0f};

		_spinner.init(ui, _spinner_holder, {.outer_color = theme.color_accent0, .inner_color = theme.color_accent1});
	}

	void editor_modal_progress_bar_t::uninit()
	{
		_spinner.uninit();
		_bar.uninit();
		_ui->deallocate_widget(_root);
		_ui				= nullptr;
		_root			= NULL_WIDGET;
		_spinner_row	= NULL_WIDGET;
		_spinner_holder = NULL_WIDGET;
	}

	void editor_modal_progress_bar_t::set_progress(f32 progress)
	{
		_progress = math::clamp(progress, 0.0f, 1.0f);
		if (_ui != nullptr)
			_bar.update_progress(_progress);
	}

	editor_modal_content_desc_t editor_modal_progress_bar_t::get_content_desc()
	{
		return {.init = init_content, .uninit = uninit_content, .user_data = this, .frame_width_x = MODAL_PROGRESS_BAR_FRAME_WIDTH_X, .fill_x = true};
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
