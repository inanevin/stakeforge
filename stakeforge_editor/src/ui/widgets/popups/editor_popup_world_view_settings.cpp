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

#include "ui/widgets/popups/editor_popup_world_view_settings.hpp"
#include "ui/panels/editor_theme.hpp"
#include "world/editor_world_view_settings.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_popup_world_view_settings_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_popup_world_view_settings_config_t& config)
	{
		SFG_ASSERT(config.settings != nullptr);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "world_view_settings_popup");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = theme.item_width * 2.0f;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		void* settings = config.settings;
		_reflection.init(ui,
						 _root,
						 {
							 .objects = {.data = &settings, .size = 1},
							 .type_id = type_id_t<editor_world_view_settings_t>::value,
						 });
	}

	void editor_popup_world_view_settings_t::uninit()
	{
		_reflection.uninit();
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}
}
