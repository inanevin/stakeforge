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

#include "commands/editor_command_animation_graph.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
#define ANIMATION_GRAPH_DUPLICATE_NODE_Y_OFFSET		 32.0f
#define ANIMATION_GRAPH_DUPLICATE_ASM_STATE_Y_OFFSET 32.0f

	namespace
	{
		chunk_handle32_t animation_graph_node_to_aux(editor_command_system_t& system, const animation_graph_node_def_t& node)
		{
			ostream_t stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_graph_node_def_t>::value, const_cast<animation_graph_node_def_t*>(&node), nullptr, stream))
			{
				SFG_ERR("failed to serialize animation graph node {0}", node.id);
				return {};
			}

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool add_animation_graph_node_from_aux(editor_command_system_t& system, editor_animation_graph_context_t& context, const editor_command_animation_graph_node_payload_t& payload)
		{
			animation_graph_node_def_t node = {};
			istream_t				   stream(system.get_aux_data().get<u8>(payload.node_stream), payload.node_stream.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_graph_node_def_t>::value, &node, nullptr, stream))
			{
				SFG_ERR("failed to deserialize animation graph node {0}", payload.node_id);
				return false;
			}

			animation_graph_def_t& graph = context.get_graph();
			graph.nodes.insert(graph.nodes.begin() + payload.node_index, std::move(node));
			return true;
		}

		chunk_handle32_t animation_graph_asm_state_to_aux(editor_command_system_t& system, const animation_graph_asm_state_def_t& state)
		{
			ostream_t stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_graph_asm_state_def_t>::value, const_cast<animation_graph_asm_state_def_t*>(&state), nullptr, stream))
			{
				SFG_ERR("failed to serialize animation graph ASM state {0}", state.id);
				return {};
			}

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));
			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool add_animation_graph_asm_state_from_aux(editor_command_system_t& system, editor_animation_graph_context_t& context, const editor_command_animation_graph_asm_state_payload_t& payload)
		{
			animation_graph_asm_state_def_t state = {};
			istream_t						stream(system.get_aux_data().get<u8>(payload.state_stream), payload.state_stream.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_graph_asm_state_def_t>::value, &state, nullptr, stream))
			{
				SFG_ERR("failed to deserialize animation graph ASM state {0}", payload.state_id);
				return false;
			}

			animation_graph_def_t& graph   = context.get_graph();
			const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.parent_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());
			SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

			node_it->asm_node.states.insert(node_it->asm_node.states.begin() + payload.state_index, std::move(state));
			return true;
		}

		bool animation_graph_add_node_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_node_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&								 graph	 = context.get_graph();
			const auto											 node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.node_id; });
			SFG_ASSERT(node_it != graph.nodes.end());

			graph.nodes.erase(node_it);
			context.set_selected_node_id(payload.previous_selection);
			return true;
		}

		bool animation_graph_add_node_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_node_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!add_animation_graph_node_from_aux(system, context, payload))
				return false;

			context.set_selected_node_id(payload.node_id);
			return true;
		}

		bool animation_graph_delete_node_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_node_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!add_animation_graph_node_from_aux(system, context, payload))
				return false;

			context.set_selected_node_id(payload.previous_selection);
			return true;
		}

		bool animation_graph_delete_node_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_node_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&								 graph	 = context.get_graph();
			const auto											 node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.node_id; });
			SFG_ASSERT(node_it != graph.nodes.end());

			graph.nodes.erase(node_it);
			context.set_selected_node_id(ANIMATION_GRAPH_DEF_NULL_ID);

			return true;
		}

		bool animation_graph_add_asm_state_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_asm_state_payload_t>(command);
			editor_animation_graph_context_t&						  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&									  graph	  = context.get_graph();
			const auto												  node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.parent_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());

			const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&payload](const animation_graph_asm_state_def_t& state) { return state.id == payload.state_id; });

			SFG_ASSERT(state_it != node_it->asm_node.states.end());

			node_it->asm_node.states.erase(state_it);
			context.set_selected_sub_node_id(payload.previous_selection);
			return true;
		}

		bool animation_graph_add_asm_state_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_asm_state_payload_t>(command);
			editor_animation_graph_context_t&						  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!add_animation_graph_asm_state_from_aux(system, context, payload))
				return false;

			context.set_selected_sub_node_id(payload.state_id);
			return true;
		}

		bool animation_graph_delete_asm_state_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_asm_state_payload_t>(command);
			editor_animation_graph_context_t&						  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!add_animation_graph_asm_state_from_aux(system, context, payload))
				return false;

			context.set_selected_sub_node_id(payload.previous_selection);
			return true;
		}

		bool animation_graph_delete_asm_state_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_asm_state_payload_t>(command);
			editor_animation_graph_context_t&						  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&									  graph	  = context.get_graph();
			const auto												  node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.parent_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());

			const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&payload](const animation_graph_asm_state_def_t& state) { return state.id == payload.state_id; });

			SFG_ASSERT(state_it != node_it->asm_node.states.end());

			node_it->asm_node.states.erase(state_it);
			context.set_selected_sub_node_id(ANIMATION_GRAPH_DEF_NULL_ID);
			return true;
		}

		bool animation_graph_select_node_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_select_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_select_node_payload_t>(command);
			editor_animation_graph_context_t&							context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_selected_node_id(payload.previous_node_id);
			context.set_selected_sub_node_id(payload.previous_sub_node_id);
			return true;
		}

		bool animation_graph_select_node_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_select_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_select_node_payload_t>(command);
			editor_animation_graph_context_t&							context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_selected_node_id(payload.post_node_id);
			context.set_selected_sub_node_id(payload.post_sub_node_id);
			return true;
		}

		bool animation_graph_set_display_mode_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_display_mode_payload_t& payload = system.get_payload_as<editor_command_animation_graph_display_mode_payload_t>(command);
			editor_animation_graph_context_t&							 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_display_mode(static_cast<editor_animation_graph_display_mode_e>(payload.previous_mode));
			context.set_display_node_id(payload.previous_display_node_id);
			context.set_selected_sub_node_id(payload.previous_sub_node_id);
			return true;
		}

		bool animation_graph_set_display_mode_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_display_mode_payload_t& payload = system.get_payload_as<editor_command_animation_graph_display_mode_payload_t>(command);
			editor_animation_graph_context_t&							 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_display_mode(static_cast<editor_animation_graph_display_mode_e>(payload.post_mode));
			context.set_display_node_id(payload.post_display_node_id);
			context.set_selected_sub_node_id(payload.post_sub_node_id);
			return true;
		}

		bool animation_graph_make_entry_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_node_payload_t>(command);
			editor_animation_graph_context_t&							   context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			context.get_graph().entry_node_id									   = payload.previous_node_id;
			return true;
		}

		bool animation_graph_make_entry_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_node_payload_t>(command);
			editor_animation_graph_context_t&							   context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			context.get_graph().entry_node_id									   = payload.post_node_id;
			return true;
		}

		bool animation_graph_make_exit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_node_payload_t>(command);
			editor_animation_graph_context_t&							   context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			context.get_graph().output_node_id									   = payload.previous_node_id;
			return true;
		}

		bool animation_graph_make_exit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_node_payload_t>(command);
			editor_animation_graph_context_t&							   context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			context.get_graph().output_node_id									   = payload.post_node_id;
			return true;
		}

		bool animation_graph_make_start_state_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_asm_state_payload_t>(command);
			editor_animation_graph_context_t&									context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&												graph	= context.get_graph();
			const auto															node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.parent_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());
			SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

			node_it->asm_node.first_state_id = payload.previous_state_id;
			return true;
		}

		bool animation_graph_make_start_state_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_designate_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_designate_asm_state_payload_t>(command);
			editor_animation_graph_context_t&									context = *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&												graph	= context.get_graph();
			const auto															node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.parent_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());
			SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

			node_it->asm_node.first_state_id = payload.post_state_id;
			return true;
		}

		bool animation_graph_connect_nodes_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_connect_nodes_payload_t& payload	= system.get_payload_as<editor_command_animation_graph_connect_nodes_payload_t>(command);
			editor_animation_graph_context_t&							  context	= *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&										  graph		= context.get_graph();
			const auto													  source_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.source_node_id; });
			SFG_ASSERT(source_it != graph.nodes.end());

			source_it->next_node_id = payload.previous_node_id;
			return true;
		}

		bool animation_graph_connect_nodes_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_connect_nodes_payload_t& payload	= system.get_payload_as<editor_command_animation_graph_connect_nodes_payload_t>(command);
			editor_animation_graph_context_t&							  context	= *static_cast<editor_animation_graph_context_t*>(command.user_data);
			animation_graph_def_t&										  graph		= context.get_graph();
			const auto													  source_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&payload](const animation_graph_node_def_t& node) { return node.id == payload.source_node_id; });
			SFG_ASSERT(source_it != graph.nodes.end());

			source_it->next_node_id = payload.post_node_id;
			return true;
		}

		bool animation_graph_node_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_animation_graph_node_payload_t& payload = system.get_payload_as<editor_command_animation_graph_node_payload_t>(command);

			if (payload.node_stream)
			{
				system.get_aux_data().free(payload.node_stream);
				payload.node_stream = {};
			}

			return true;
		}

		bool animation_graph_asm_state_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_animation_graph_asm_state_payload_t& payload = system.get_payload_as<editor_command_animation_graph_asm_state_payload_t>(command);

			if (payload.state_stream)
			{
				system.get_aux_data().free(payload.state_stream);
				payload.state_stream = {};
			}

			return true;
		}
	}

	bool editor_command_animation_graph_add_node_t::add(editor_animation_graph_context_t& context, animation_graph_node_type_e type, const vec2f_t& editor_position, const char* name)
	{
		animation_graph_def_t&			 graph	 = context.get_graph();
		const u32						 node_id = context.acquire_node_id();
		const animation_graph_node_def_t node{
			.name			 = name,
			.editor_position = editor_position,
			.id				 = node_id,
			.type			 = type,
		};

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 node_stream	= animation_graph_node_to_aux(command_system, node);

		if (!node_stream)
			return false;

		const editor_command_animation_graph_node_payload_t payload{
			.node_stream		= node_stream,
			.node_id			= node_id,
			.node_index			= static_cast<u32>(graph.nodes.size()),
			.previous_selection = context.get_selected_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_add_node_undo,
			.redo		= animation_graph_add_node_redo,
			.cleanup	= animation_graph_node_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Add Node",
			.type		= editor_command_type_e::animation_graph_add_node,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph add node command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_delete_node_t::remove(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });
		SFG_ASSERT(node_it != graph.nodes.end());

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 node_stream	= animation_graph_node_to_aux(command_system, *node_it);

		if (!node_stream)
			return false;

		const editor_command_animation_graph_node_payload_t payload{
			.node_stream		= node_stream,
			.node_id			= node_id,
			.node_index			= static_cast<u32>(node_it - graph.nodes.begin()),
			.previous_selection = context.get_selected_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_delete_node_undo,
			.redo		= animation_graph_delete_node_redo,
			.cleanup	= animation_graph_node_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Delete Node",
			.type		= editor_command_type_e::animation_graph_delete_node,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph delete node command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_duplicate_node_t::duplicate(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });
		SFG_ASSERT(node_it != graph.nodes.end());

		animation_graph_node_def_t node = *node_it;
		node.id							= context.acquire_node_id();
		node.editor_position.y += ANIMATION_GRAPH_DUPLICATE_NODE_Y_OFFSET;

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 node_stream	= animation_graph_node_to_aux(command_system, node);

		if (!node_stream)
			return false;

		const editor_command_animation_graph_node_payload_t payload{
			.node_stream		= node_stream,
			.node_id			= node.id,
			.node_index			= static_cast<u32>(graph.nodes.size()),
			.previous_selection = context.get_selected_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_add_node_undo,
			.redo		= animation_graph_add_node_redo,
			.cleanup	= animation_graph_node_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Duplicate Node",
			.type		= editor_command_type_e::animation_graph_duplicate_node,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph duplicate node command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_add_asm_state_t::add(editor_animation_graph_context_t& context, const vec2f_t& editor_position, const char* name)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const u32							  state_id = context.acquire_node_id();
		const animation_graph_asm_state_def_t state{
			.name			 = name,
			.editor_position = editor_position,
			.id				 = state_id,
		};

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 state_stream	= animation_graph_asm_state_to_aux(command_system, state);

		if (!state_stream)
			return false;

		const editor_command_animation_graph_asm_state_payload_t payload{
			.state_stream		= state_stream,
			.parent_node_id		= node_id,
			.state_id			= state_id,
			.state_index		= static_cast<u32>(node_it->asm_node.states.size()),
			.previous_selection = context.get_selected_sub_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_add_asm_state_undo,
			.redo		= animation_graph_add_asm_state_redo,
			.cleanup	= animation_graph_asm_state_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Add ASM State",
			.type		= editor_command_type_e::animation_graph_add_asm_state,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph add ASM state command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_delete_asm_state_t::remove(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; });

		SFG_ASSERT(state_it != node_it->asm_node.states.end());

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 state_stream	= animation_graph_asm_state_to_aux(command_system, *state_it);

		if (!state_stream)
			return false;

		const editor_command_animation_graph_asm_state_payload_t payload{
			.state_stream		= state_stream,
			.parent_node_id		= node_id,
			.state_id			= state_id,
			.state_index		= static_cast<u32>(state_it - node_it->asm_node.states.begin()),
			.previous_selection = context.get_selected_sub_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_delete_asm_state_undo,
			.redo		= animation_graph_delete_asm_state_redo,
			.cleanup	= animation_graph_asm_state_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Delete ASM State",
			.type		= editor_command_type_e::animation_graph_delete_asm_state,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph delete ASM state command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_duplicate_asm_state_t::duplicate(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; });

		SFG_ASSERT(state_it != node_it->asm_node.states.end());

		animation_graph_asm_state_def_t state = *state_it;
		state.id							  = context.acquire_node_id();
		state.editor_position.y += ANIMATION_GRAPH_DUPLICATE_ASM_STATE_Y_OFFSET;

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 state_stream	= animation_graph_asm_state_to_aux(command_system, state);

		if (!state_stream)
			return false;

		const editor_command_animation_graph_asm_state_payload_t payload{
			.state_stream		= state_stream,
			.parent_node_id		= node_id,
			.state_id			= state.id,
			.state_index		= static_cast<u32>(node_it->asm_node.states.size()),
			.previous_selection = context.get_selected_sub_node_id(),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_add_asm_state_undo,
			.redo		= animation_graph_add_asm_state_redo,
			.cleanup	= animation_graph_asm_state_cleanup,
			.user_data	= &context,
			.debug_name = "Animation Graph Duplicate ASM State",
			.type		= editor_command_type_e::animation_graph_duplicate_asm_state,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph duplicate ASM state command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_select_node_t::select(editor_animation_graph_context_t& context, u32 node_id)
	{
		if (context.get_selected_node_id() == node_id)
			return true;

		const editor_command_animation_graph_select_node_payload_t payload{
			.previous_node_id	  = context.get_selected_node_id(),
			.previous_sub_node_id = context.get_selected_sub_node_id(),
			.post_node_id		  = node_id,
			.post_sub_node_id	  = ANIMATION_GRAPH_DEF_NULL_ID,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_select_node_undo,
			.redo		= animation_graph_select_node_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Select Node",
			.type		= editor_command_type_e::animation_graph_select_node,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph select node command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_select_node_t::select_sub_node(editor_animation_graph_context_t& context, u32 node_id)
	{
		if (context.get_selected_sub_node_id() == node_id)
			return true;

		const editor_command_animation_graph_select_node_payload_t payload{
			.previous_node_id	  = context.get_selected_node_id(),
			.previous_sub_node_id = context.get_selected_sub_node_id(),
			.post_node_id		  = context.get_selected_node_id(),
			.post_sub_node_id	  = node_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_select_node_undo,
			.redo		= animation_graph_select_node_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Select Sub Node",
			.type		= editor_command_type_e::animation_graph_select_node,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph select sub node command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_set_display_mode_t::set(editor_animation_graph_context_t& context, editor_animation_graph_display_mode_e mode, u32 display_node_id)
	{
		if (context.get_display_mode() == mode && context.get_display_node_id() == display_node_id)
			return true;

		const editor_command_animation_graph_display_mode_payload_t payload{
			.previous_display_node_id = context.get_display_node_id(),
			.post_display_node_id	  = display_node_id,
			.previous_sub_node_id	  = context.get_selected_sub_node_id(),
			.post_sub_node_id		  = ANIMATION_GRAPH_DEF_NULL_ID,
			.previous_mode			  = static_cast<u8>(context.get_display_mode()),
			.post_mode				  = static_cast<u8>(mode),
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_set_display_mode_undo,
			.redo		= animation_graph_set_display_mode_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Set Display Mode",
			.type		= editor_command_type_e::animation_graph_set_display_mode,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph set display mode command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_make_entry_t::make(editor_animation_graph_context_t& context, u32 node_id)
	{
		if (context.get_graph().entry_node_id == node_id)
			return true;

		const editor_command_animation_graph_designate_node_payload_t payload{
			.previous_node_id = context.get_graph().entry_node_id,
			.post_node_id	  = node_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_make_entry_undo,
			.redo		= animation_graph_make_entry_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Make Entry",
			.type		= editor_command_type_e::animation_graph_make_entry,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph make entry command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_make_exit_t::make(editor_animation_graph_context_t& context, u32 node_id)
	{
		if (context.get_graph().output_node_id == node_id)
			return true;

		const editor_command_animation_graph_designate_node_payload_t payload{
			.previous_node_id = context.get_graph().output_node_id,
			.post_node_id	  = node_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_make_exit_undo,
			.redo		= animation_graph_make_exit_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Make Exit",
			.type		= editor_command_type_e::animation_graph_make_exit,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph make exit command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_make_start_state_t::make(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph		  = context.get_graph();
		const u32			   parent_node_id = context.get_display_node_id();
		const auto			   node_it		  = std::find_if(graph.nodes.begin(), graph.nodes.end(), [parent_node_id](const animation_graph_node_def_t& node) { return node.id == parent_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);
		SFG_ASSERT(std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; }) != node_it->asm_node.states.end());

		if (node_it->asm_node.first_state_id == state_id)
			return true;

		const editor_command_animation_graph_designate_asm_state_payload_t payload{
			.parent_node_id	   = parent_node_id,
			.previous_state_id = node_it->asm_node.first_state_id,
			.post_state_id	   = state_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_make_start_state_undo,
			.redo		= animation_graph_make_start_state_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Make Start State",
			.type		= editor_command_type_e::animation_graph_make_start_state,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph make start state command");
			return false;
		}

		return true;
	}

	bool editor_command_animation_graph_connect_nodes_t::connect(editor_animation_graph_context_t& context, u32 source_node_id, u32 target_node_id)
	{
		animation_graph_def_t& graph	 = context.get_graph();
		const auto			   source_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [source_node_id](const animation_graph_node_def_t& node) { return node.id == source_node_id; });
		SFG_ASSERT(source_it != graph.nodes.end());
		SFG_ASSERT(std::find_if(graph.nodes.begin(), graph.nodes.end(), [target_node_id](const animation_graph_node_def_t& node) { return node.id == target_node_id; }) != graph.nodes.end());

		if (source_it->next_node_id == target_node_id)
			return true;

		const editor_command_animation_graph_connect_nodes_payload_t payload{
			.source_node_id	  = source_node_id,
			.previous_node_id = source_it->next_node_id,
			.post_node_id	  = target_node_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_connect_nodes_undo,
			.redo		= animation_graph_connect_nodes_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Connect Nodes",
			.type		= editor_command_type_e::animation_graph_connect_nodes,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph connect nodes command");
			return false;
		}

		return true;
	}
}
