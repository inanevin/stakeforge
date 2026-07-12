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
#include "ui/widgets/inspector/editor_widget_inspector.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_command_system.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_inspector_config_t& config)
	{
		_ui					 = &ui;
		_allow_prefab_blocks = config.allow_prefab_blocks;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "entity_inspector");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "inspector_column");
		tree.attach(_root, _column);

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		_command_listener = editor_command_system_t::get().add_listener(on_command_system_event, this);
	}

	void editor_widget_inspector_t::uninit()
	{
		editor_command_system_t::get().remove_listener(_command_listener);
		_ui->cancel_mutations(this);
		clear_display();
		_ui->deallocate_widget(_root);
		_component_states.clear();
		_field_states.clear();
		_display_entities.clear();
		_column = NULL_WIDGET;
		_copied_component_stream.destroy();
		_copied_entity_info		   = {};
		_command_listener		   = {};
		_edit_world				   = {};
		_copied_component_type	   = 0;
		_action_menu_type_id	   = 0;
		_pending_component_type	   = 0;
		_refresh_component_pending = false;
		_copied_entity_info_valid  = false;
		_allow_prefab_blocks	   = false;

		_root = NULL_WIDGET;
		_ui	  = nullptr;
	}

	void editor_widget_inspector_t::set_edit_world(editor_world_handle_t world)
	{
		_edit_world = world;
	}

}
