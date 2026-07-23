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

#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_grid.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_widget_inspector.hpp"

#include <sfg/io/assert.hpp>

namespace sfg
{
	void editor_animation_graph_context_t::init(editor_animation_graph_grid_t* grid, editor_animation_graph_widget_inspector_t* inspector)
	{
		SFG_ASSERT(grid != nullptr);
		SFG_ASSERT(inspector != nullptr);

		_grid			  = grid;
		_inspector		  = inspector;
		_command_listener = editor_command_system_t::get().add_listener(on_command_system_event, this);
	}

	void editor_animation_graph_context_t::uninit()
	{
		editor_command_system_t::get().remove_listener(_command_listener);

		_grid				  = nullptr;
		_inspector			  = nullptr;
		_command_listener	  = {};
		_graph				  = {};
		_display_node_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		_selected_node_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		_selected_sub_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		_id_counter			  = 1;
		_display_mode		  = editor_animation_graph_display_mode_e::display_nodes;
	}

	void editor_animation_graph_context_t::set_display_mode(editor_animation_graph_display_mode_e mode)
	{
		_display_mode = mode;
	}

	void editor_animation_graph_context_t::set_display_node_id(u32 node_id)
	{
		_display_node_id = node_id;
	}

	void editor_animation_graph_context_t::set_selected_node_id(u32 node_id)
	{
		_selected_node_id = node_id;
	}

	void editor_animation_graph_context_t::set_selected_sub_node_id(u32 node_id)
	{
		_selected_sub_node_id = node_id;
	}

	u32 editor_animation_graph_context_t::acquire_node_id()
	{
		_id_counter = _graph.next_id;

		++_id_counter;
		_graph.next_id = _id_counter;
		return _id_counter - 1;
	}

	void editor_animation_graph_context_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_animation_graph_context_t& context = *static_cast<editor_animation_graph_context_t*>(user_data);

		if (command.user_data != &context)
			return;

		switch (command.type)
		{
		case editor_command_type_e::animation_graph_add_node:
		case editor_command_type_e::animation_graph_delete_node:
		case editor_command_type_e::animation_graph_duplicate_node:
		case editor_command_type_e::animation_graph_add_asm_state:
		case editor_command_type_e::animation_graph_delete_asm_state:
		case editor_command_type_e::animation_graph_duplicate_asm_state:
			context._grid->refresh_nodes();
			context._inspector->refresh_inspector();
			break;
		case editor_command_type_e::animation_graph_select_node: {
			const u32 selected_node_id = context._display_mode == editor_animation_graph_display_mode_e::display_nodes ? context._selected_node_id : context._selected_sub_node_id;

			context._grid->change_selection(selected_node_id);
			context._inspector->refresh_inspector();
			break;
		}
		case editor_command_type_e::animation_graph_set_display_mode:
			context._grid->set_mode(context._display_mode);
			context._inspector->refresh_inspector();
			break;
		default:
			break;
		}
	}
}
