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

#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_reflection_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_reflection_config_t& config)
	{
		_ui = &ui;

		ui::layout_tree_t& tree	 = ui.get_tree();
		editor_theme_t&	   theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reflection");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		set_reflection(config);
	}

	void editor_widget_reflection_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_widget_reflection_t::set_reflection(const editor_widget_reflection_config_t&)
	{
	}
}
