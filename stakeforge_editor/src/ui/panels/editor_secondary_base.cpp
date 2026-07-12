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
#include "ui/panels/editor_secondary_base.hpp"
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_secondary_base_t::init(ui::ui_context& ui, ui::widget_id_t parent, window_runtime_t& runtime)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui						= &ui;
		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "secondary_base");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		dock_widget_config_t dock_config   = {};
		dock_config.runtime				   = &runtime;
		dock_config.root_drag_out_behavior = dock_widget_root_drag_out_e::close_window;
		_dock_widget.init(ui, _root, dock_config);
		ui::layout_in_t& dock_in = tree.in(_dock_widget.get_root());
		dock_in.size_mode_y		 = ui::axis_mode_e::fill;
	}

	void editor_secondary_base_t::uninit()
	{
		_dock_widget.uninit();

		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

}
