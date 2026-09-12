/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "commands/editor_command_animation_graph.hpp"
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
		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_animation_graph_edit_t::cancel(*this);
		command_system.clear_user_data(this);
		command_system.remove_listener(_command_listener);

		_grid								  = nullptr;
		_inspector							  = nullptr;
		_command_listener					  = {};
		_graph								  = {};
		_asset_id							  = NULL_SID;
		_display_node_id					  = ANIMATION_GRAPH_DEF_NULL_ID;
		_selected_node_id					  = ANIMATION_GRAPH_DEF_NULL_ID;
		_selected_sub_node_id				  = ANIMATION_GRAPH_DEF_NULL_ID;
		_selected_transition_id				  = ANIMATION_GRAPH_DEF_NULL_ID;
		_edit_previous_display_node_id		  = ANIMATION_GRAPH_DEF_NULL_ID;
		_edit_previous_selected_node_id		  = ANIMATION_GRAPH_DEF_NULL_ID;
		_edit_previous_selected_sub_node_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		_edit_previous_selected_transition_id = ANIMATION_GRAPH_DEF_NULL_ID;
		_id_counter							  = 1;
		_edit_previous_stream				  = {};
		_display_mode						  = editor_animation_graph_display_mode_e::display_nodes;
		_edit_previous_mode					  = editor_animation_graph_display_mode_e::display_nodes;
	}

	void editor_animation_graph_context_t::set_display_mode(editor_animation_graph_display_mode_e mode)
	{
		_display_mode = mode;
	}

	void editor_animation_graph_context_t::set_asset_id(sid_t asset_id)
	{
		_asset_id = asset_id;
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

	void editor_animation_graph_context_t::set_selected_transition_id(u32 transition_id)
	{
		_selected_transition_id = transition_id;
	}

	u32 editor_animation_graph_context_t::acquire_node_id()
	{
		_id_counter = _graph.next_id;

		for (const animation_graph_node_def_t& node : _graph.nodes)
		{
			_id_counter = std::max(_id_counter, node.id + 1);

			if (node.type != animation_graph_node_type_e::asm_node)
				continue;

			for (const animation_graph_asm_state_def_t& state : node.asm_node.states)
				_id_counter = std::max(_id_counter, state.id + 1);

			for (const animation_graph_asm_transition_def_t& transition : node.asm_node.transitions)
				_id_counter = std::max(_id_counter, transition.id + 1);
		}

		const u32 acquired_id = _id_counter;

		++_id_counter;
		_graph.next_id = _id_counter;
		return acquired_id;
	}

	void editor_animation_graph_context_t::ensure_graph_node_designations()
	{
		if (_graph.nodes.empty())
		{
			_graph.entry_node_id  = ANIMATION_GRAPH_DEF_NULL_ID;
			_graph.output_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
			return;
		}

		const auto entry_it = std::find_if(_graph.nodes.begin(), _graph.nodes.end(), [this](const animation_graph_node_def_t& node) { return node.id == _graph.entry_node_id; });

		if (entry_it == _graph.nodes.end())
			_graph.entry_node_id = _graph.nodes.front().id;

		const auto output_it = std::find_if(_graph.nodes.begin(), _graph.nodes.end(), [this](const animation_graph_node_def_t& node) { return node.id == _graph.output_node_id; });

		if (output_it == _graph.nodes.end())
			_graph.output_node_id = _graph.nodes.back().id;
	}

	void editor_animation_graph_context_t::ensure_asm_state_designations()
	{
		for (animation_graph_node_def_t& node : _graph.nodes)
		{
			if (node.type != animation_graph_node_type_e::asm_node)
				continue;

			if (node.asm_node.states.empty())
			{
				node.asm_node.first_state_id = ANIMATION_GRAPH_DEF_NULL_ID;
				continue;
			}

			const auto first_state_it = std::find_if(node.asm_node.states.begin(), node.asm_node.states.end(), [&node](const animation_graph_asm_state_def_t& state) { return state.id == node.asm_node.first_state_id; });

			if (first_state_it == node.asm_node.states.end())
				node.asm_node.first_state_id = node.asm_node.states.front().id;
		}
	}

	void editor_animation_graph_context_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_animation_graph_context_t& context = *static_cast<editor_animation_graph_context_t*>(user_data);

		if (command.user_data != &context)
			return;

		switch (command.type)
		{
		case editor_command_type_e::animation_graph_edit: {
			context._grid->refresh_nodes();

			const u32 selected_node_id = context._display_mode == editor_animation_graph_display_mode_e::display_nodes ? context._selected_node_id : context._selected_sub_node_id;

			context._grid->change_selection(selected_node_id);
			context._inspector->refresh_inspector();
			break;
		}
		case editor_command_type_e::animation_graph_select_transition:
			context._grid->change_selection(context._selected_sub_node_id);
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
