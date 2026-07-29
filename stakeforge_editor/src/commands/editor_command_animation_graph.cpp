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
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_grid.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ANIMATION_GRAPH_DUPLICATE_NODE_Y_OFFSET		 32.0f
#define ANIMATION_GRAPH_DUPLICATE_ASM_STATE_Y_OFFSET 32.0f

	namespace
	{
		editor_animation_graph_navigation_state_t capture_navigation(const editor_animation_graph_context_t& context)
		{
			return {
				.display_node_id		= context.get_display_node_id(),
				.selected_node_id		= context.get_selected_node_id(),
				.selected_sub_node_id	= context.get_selected_sub_node_id(),
				.selected_transition_id = context.get_selected_transition_id(),
				.mode					= static_cast<u8>(context.get_display_mode()),
			};
		}

		void apply_navigation(editor_animation_graph_context_t& context, const editor_animation_graph_navigation_state_t& state)
		{
			context.set_display_node_id(state.display_node_id);
			context.set_selected_node_id(state.selected_node_id);
			context.set_selected_sub_node_id(state.selected_sub_node_id);
			context.set_selected_transition_id(state.selected_transition_id);
			context.set_display_mode(static_cast<editor_animation_graph_display_mode_e>(state.mode));
		}

		bool is_same_navigation(const editor_animation_graph_navigation_state_t& a, const editor_animation_graph_navigation_state_t& b)
		{
			return a.display_node_id == b.display_node_id && a.selected_node_id == b.selected_node_id && a.selected_sub_node_id == b.selected_sub_node_id && a.selected_transition_id == b.selected_transition_id && a.mode == b.mode;
		}

		void save_and_cook_animation_graph_async(editor_animation_graph_context_t& context)
		{
			nlohmann::json		   embedded_source = nlohmann::json::object();
			animation_graph_def_t& graph		   = context.get_graph();

			if (!reflection_registry_t::get().type_to_json(type_id_t<animation_graph_def_t>::value, &graph, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize animation graph definition for asset {0}", context.get_asset_id());
				return;
			}

			embedded_source["schema"] = "sfg.schema.animation_graph";
			editor_asset_manager_t::get().save_and_cook_embedded_asset_async(context.get_asset_id(), embedded_source);
		}

		chunk_handle32_t animation_graph_to_aux(editor_command_system_t& system, const animation_graph_def_t& graph)
		{
			ostream_t stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_graph_def_t>::value, const_cast<animation_graph_def_t*>(&graph), nullptr, stream))
			{
				SFG_ERR("failed to serialize animation graph");
				return {};
			}

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));

			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool animation_graph_from_aux(editor_command_system_t& system, editor_animation_graph_context_t& context, chunk_handle32_t handle)
		{
			animation_graph_def_t graph = {};
			istream_t			  stream(system.get_aux_data().get<u8>(handle), handle.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_graph_def_t>::value, &graph, nullptr, stream))
			{
				SFG_ERR("failed to deserialize animation graph");
				return false;
			}

			context.get_graph() = std::move(graph);
			return true;
		}

		bool animation_graph_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_edit_payload_t& payload = system.get_payload_as<editor_command_animation_graph_edit_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!animation_graph_from_aux(system, context, payload.previous_stream))
				return false;

			apply_navigation(context, payload.previous_navigation);

			if (payload.graph_changed)
				save_and_cook_animation_graph_async(context);

			return true;
		}

		bool animation_graph_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_edit_payload_t& payload = system.get_payload_as<editor_command_animation_graph_edit_payload_t>(command);
			editor_animation_graph_context_t&					 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			if (!animation_graph_from_aux(system, context, payload.post_stream))
				return false;

			apply_navigation(context, payload.post_navigation);

			if (payload.graph_changed)
				save_and_cook_animation_graph_async(context);

			return true;
		}

		bool animation_graph_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_animation_graph_edit_payload_t& payload = system.get_payload_as<editor_command_animation_graph_edit_payload_t>(command);

			system.get_aux_data().free(payload.previous_stream);
			system.get_aux_data().free(payload.post_stream);
			payload.previous_stream = {};
			payload.post_stream		= {};
			return true;
		}

		bool animation_graph_select_transition_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_select_transition_payload_t& payload = system.get_payload_as<editor_command_animation_graph_select_transition_payload_t>(command);
			editor_animation_graph_context_t&								  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_selected_transition_id(payload.previous_transition_id);
			context.set_selected_sub_node_id(payload.previous_sub_node_id);
			return true;
		}

		bool animation_graph_select_transition_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_select_transition_payload_t& payload = system.get_payload_as<editor_command_animation_graph_select_transition_payload_t>(command);
			editor_animation_graph_context_t&								  context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_selected_transition_id(payload.post_transition_id);
			context.set_selected_sub_node_id(payload.post_sub_node_id);
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
			context.set_selected_transition_id(payload.previous_transition_id);
			return true;
		}

		bool animation_graph_set_display_mode_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_graph_display_mode_payload_t& payload = system.get_payload_as<editor_command_animation_graph_display_mode_payload_t>(command);
			editor_animation_graph_context_t&							 context = *static_cast<editor_animation_graph_context_t*>(command.user_data);

			context.set_display_mode(static_cast<editor_animation_graph_display_mode_e>(payload.post_mode));
			context.set_display_node_id(payload.post_display_node_id);
			context.set_selected_sub_node_id(payload.post_sub_node_id);
			context.set_selected_transition_id(payload.post_transition_id);
			return true;
		}
	}

	bool editor_command_animation_graph_edit_t::begin(editor_animation_graph_context_t& context)
	{
		SFG_ASSERT(!context._edit_previous_stream);

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 stream			= animation_graph_to_aux(command_system, context.get_graph());

		if (!stream)
			return false;

		const editor_animation_graph_navigation_state_t navigation = capture_navigation(context);

		context._edit_previous_stream				  = stream;
		context._edit_previous_display_node_id		  = navigation.display_node_id;
		context._edit_previous_selected_node_id		  = navigation.selected_node_id;
		context._edit_previous_selected_sub_node_id	  = navigation.selected_sub_node_id;
		context._edit_previous_selected_transition_id = navigation.selected_transition_id;
		context._edit_previous_mode					  = static_cast<editor_animation_graph_display_mode_e>(navigation.mode);
		return true;
	}

	bool editor_command_animation_graph_edit_t::submit(editor_animation_graph_context_t& context, const char* debug_name, bool notify)
	{
		SFG_ASSERT(context._edit_previous_stream);

		context.ensure_graph_node_designations();
		context.ensure_asm_state_designations();

		editor_command_system_t&						command_system = editor_command_system_t::get();
		const chunk_handle32_t							post_stream	   = animation_graph_to_aux(command_system, context.get_graph());
		const editor_animation_graph_navigation_state_t previous_navigation{
			.display_node_id		= context._edit_previous_display_node_id,
			.selected_node_id		= context._edit_previous_selected_node_id,
			.selected_sub_node_id	= context._edit_previous_selected_sub_node_id,
			.selected_transition_id = context._edit_previous_selected_transition_id,
			.mode					= static_cast<u8>(context._edit_previous_mode),
		};
		const editor_animation_graph_navigation_state_t post_navigation = capture_navigation(context);

		if (!post_stream)
		{
			animation_graph_from_aux(command_system, context, context._edit_previous_stream);
			apply_navigation(context, previous_navigation);
			command_system.get_aux_data().free(context._edit_previous_stream);
			context._edit_previous_stream = {};
			return false;
		}

		const bool is_same_graph = context._edit_previous_stream.size == post_stream.size && SFG_MEMCMP(command_system.get_aux_data().get<u8>(context._edit_previous_stream), command_system.get_aux_data().get<u8>(post_stream), post_stream.size) == 0;

		if (is_same_graph && is_same_navigation(previous_navigation, post_navigation))
		{
			command_system.get_aux_data().free(context._edit_previous_stream);
			command_system.get_aux_data().free(post_stream);
			context._edit_previous_stream = {};
			return true;
		}

		const editor_command_animation_graph_edit_payload_t payload{
			.previous_navigation = previous_navigation,
			.post_navigation	 = post_navigation,
			.previous_stream	 = context._edit_previous_stream,
			.post_stream		 = post_stream,
			.graph_changed		 = !is_same_graph,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_edit_undo,
			.redo		= animation_graph_edit_redo,
			.cleanup	= animation_graph_edit_cleanup,
			.user_data	= &context,
			.debug_name = debug_name,
			.type		= editor_command_type_e::animation_graph_edit,
			.run_redo	= false,
			.notify		= notify,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			animation_graph_from_aux(command_system, context, context._edit_previous_stream);
			apply_navigation(context, payload.previous_navigation);
			command_system.get_aux_data().free(context._edit_previous_stream);
			command_system.get_aux_data().free(post_stream);
			context._edit_previous_stream = {};
			SFG_ERR("failed to issue animation graph edit command");
			return false;
		}

		context._edit_previous_stream = {};

		if (payload.graph_changed)
		{
			if (!notify)
				context._grid->refresh_node_titles();

			save_and_cook_animation_graph_async(context);
		}

		return true;
	}

	void editor_command_animation_graph_edit_t::cancel(editor_animation_graph_context_t& context)
	{
		if (!context._edit_previous_stream)
			return;

		editor_command_system_t::get().get_aux_data().free(context._edit_previous_stream);
		context._edit_previous_stream = {};
	}

	bool editor_command_animation_graph_edit_t::add_node(editor_animation_graph_context_t& context, animation_graph_node_type_e type, const vec2f_t& editor_position, const char* name)
	{
		if (!begin(context))
			return false;

		animation_graph_def_t&	   graph   = context.get_graph();
		const u32				   node_id = context.acquire_node_id();
		animation_graph_node_def_t node{
			.name			 = name,
			.editor_position = editor_position,
			.id				 = node_id,
			.type			 = type,
		};

		if (type == animation_graph_node_type_e::asm_node)
		{
			const u32 state_id = context.acquire_node_id();

			node.asm_node.states.push_back({
				.name			 = "State",
				.editor_position = vec2f_t::zero,
				.id				 = state_id,
			});
			node.asm_node.first_state_id = state_id;
		}

		graph.nodes.push_back(std::move(node));
		context.set_selected_node_id(node_id);

		return submit(context, "Animation Graph Add Node", true);
	}

	bool editor_command_animation_graph_edit_t::delete_node(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());

		if (graph.nodes.size() == 1)
			return false;

		if (!begin(context))
			return false;

		graph.nodes.erase(node_it);
		context.set_selected_node_id(ANIMATION_GRAPH_DEF_NULL_ID);

		return submit(context, "Animation Graph Delete Node", true);
	}

	bool editor_command_animation_graph_edit_t::duplicate_node(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());

		if (!begin(context))
			return false;

		animation_graph_node_def_t node = *node_it;

		node.id = context.acquire_node_id();
		node.editor_position.y += ANIMATION_GRAPH_DUPLICATE_NODE_Y_OFFSET;
		context.set_selected_node_id(node.id);
		graph.nodes.push_back(std::move(node));

		return submit(context, "Animation Graph Duplicate Node", true);
	}

	bool editor_command_animation_graph_edit_t::add_asm_state(editor_animation_graph_context_t& context, const vec2f_t& editor_position, const char* name)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		if (!begin(context))
			return false;

		const u32 state_id = context.acquire_node_id();

		node_it->asm_node.states.push_back({
			.name			 = name,
			.editor_position = editor_position,
			.id				 = state_id,
		});
		context.set_selected_sub_node_id(state_id);

		return submit(context, "Animation Graph Add ASM State", true);
	}

	bool editor_command_animation_graph_edit_t::delete_asm_state(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; });

		SFG_ASSERT(state_it != node_it->asm_node.states.end());

		if (node_it->asm_node.states.size() == 1)
			return false;

		if (!begin(context))
			return false;

		node_it->asm_node.transitions.erase(
			std::remove_if(node_it->asm_node.transitions.begin(), node_it->asm_node.transitions.end(), [state_id](const animation_graph_asm_transition_def_t& transition) { return transition.from_state_id == state_id || transition.to_state_id == state_id; }),
			node_it->asm_node.transitions.end());
		node_it->asm_node.states.erase(state_it);
		context.set_selected_sub_node_id(ANIMATION_GRAPH_DEF_NULL_ID);
		context.set_selected_transition_id(ANIMATION_GRAPH_DEF_NULL_ID);

		return submit(context, "Animation Graph Delete ASM State", true);
	}

	bool editor_command_animation_graph_edit_t::duplicate_asm_state(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph   = context.get_graph();
		const u32			   node_id = context.get_display_node_id();
		const auto			   node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [node_id](const animation_graph_node_def_t& node) { return node.id == node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const auto state_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; });

		SFG_ASSERT(state_it != node_it->asm_node.states.end());

		if (!begin(context))
			return false;

		animation_graph_asm_state_def_t state = *state_it;

		state.id = context.acquire_node_id();
		state.editor_position.y += ANIMATION_GRAPH_DUPLICATE_ASM_STATE_Y_OFFSET;
		context.set_selected_sub_node_id(state.id);
		node_it->asm_node.states.push_back(std::move(state));

		return submit(context, "Animation Graph Duplicate ASM State", true);
	}

	bool editor_command_animation_graph_edit_t::add_asm_transition(editor_animation_graph_context_t& context, u32 from_state_id, u32 to_state_id)
	{
		animation_graph_def_t& graph		  = context.get_graph();
		const u32			   parent_node_id = context.get_display_node_id();
		const auto			   node_it		  = std::find_if(graph.nodes.begin(), graph.nodes.end(), [parent_node_id](const animation_graph_node_def_t& node) { return node.id == parent_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);
		SFG_ASSERT(std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [from_state_id](const animation_graph_asm_state_def_t& state) { return state.id == from_state_id; }) != node_it->asm_node.states.end());
		SFG_ASSERT(std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [to_state_id](const animation_graph_asm_state_def_t& state) { return state.id == to_state_id; }) != node_it->asm_node.states.end());

		if (!begin(context))
			return false;

		const u32 transition_id = context.acquire_node_id();

		node_it->asm_node.transitions.push_back({
			.id			   = transition_id,
			.from_state_id = from_state_id,
			.to_state_id   = to_state_id,
		});
		context.set_selected_transition_id(transition_id);
		context.set_selected_sub_node_id(ANIMATION_GRAPH_DEF_NULL_ID);

		return submit(context, "Animation Graph Add ASM Transition", false);
	}

	bool editor_command_animation_graph_edit_t::delete_asm_transition(editor_animation_graph_context_t& context, u32 transition_id)
	{
		animation_graph_def_t& graph		  = context.get_graph();
		const u32			   parent_node_id = context.get_display_node_id();
		const auto			   node_it		  = std::find_if(graph.nodes.begin(), graph.nodes.end(), [parent_node_id](const animation_graph_node_def_t& node) { return node.id == parent_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);

		const auto transition_it = std::find_if(node_it->asm_node.transitions.begin(), node_it->asm_node.transitions.end(), [transition_id](const animation_graph_asm_transition_def_t& transition) { return transition.id == transition_id; });

		SFG_ASSERT(transition_it != node_it->asm_node.transitions.end());

		if (!begin(context))
			return false;

		node_it->asm_node.transitions.erase(transition_it);
		context.set_selected_transition_id(ANIMATION_GRAPH_DEF_NULL_ID);

		return submit(context, "Animation Graph Delete ASM Transition", false);
	}

	bool editor_command_animation_graph_edit_t::make_entry(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph = context.get_graph();

		if (graph.entry_node_id == node_id)
			return true;

		if (!begin(context))
			return false;

		graph.entry_node_id = node_id;

		return submit(context, "Animation Graph Make Entry", false);
	}

	bool editor_command_animation_graph_edit_t::make_exit(editor_animation_graph_context_t& context, u32 node_id)
	{
		animation_graph_def_t& graph = context.get_graph();

		if (graph.output_node_id == node_id)
			return true;

		if (!begin(context))
			return false;

		graph.output_node_id = node_id;

		return submit(context, "Animation Graph Make Exit", false);
	}

	bool editor_command_animation_graph_edit_t::make_start_state(editor_animation_graph_context_t& context, u32 state_id)
	{
		animation_graph_def_t& graph		  = context.get_graph();
		const u32			   parent_node_id = context.get_display_node_id();
		const auto			   node_it		  = std::find_if(graph.nodes.begin(), graph.nodes.end(), [parent_node_id](const animation_graph_node_def_t& node) { return node.id == parent_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);
		SFG_ASSERT(std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [state_id](const animation_graph_asm_state_def_t& state) { return state.id == state_id; }) != node_it->asm_node.states.end());

		if (node_it->asm_node.first_state_id == state_id)
			return true;

		if (!begin(context))
			return false;

		node_it->asm_node.first_state_id = state_id;

		return submit(context, "Animation Graph Make Start State", false);
	}

	bool editor_command_animation_graph_edit_t::connect_nodes(editor_animation_graph_context_t& context, u32 source_node_id, u32 target_node_id)
	{
		animation_graph_def_t& graph	 = context.get_graph();
		const auto			   source_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [source_node_id](const animation_graph_node_def_t& node) { return node.id == source_node_id; });
		const auto			   target_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [target_node_id](const animation_graph_node_def_t& node) { return node.id == target_node_id; });

		SFG_ASSERT(source_it != graph.nodes.end());
		SFG_ASSERT(target_it != graph.nodes.end());

		if (source_it->next_node_id == target_node_id)
			return true;

		const animation_graph_node_def_t* flow_node		  = &*target_it;
		size_t							  flow_node_count = 0;

		while (flow_node != nullptr && flow_node_count < graph.nodes.size())
		{
			if (flow_node->id == source_node_id)
				return false;

			flow_node = graph.find_node(flow_node->next_node_id);
			++flow_node_count;
		}

		if (flow_node != nullptr)
			return false;

		if (!begin(context))
			return false;

		source_it->next_node_id = target_node_id;

		return submit(context, "Animation Graph Connect Nodes", false);
	}

	bool editor_command_animation_graph_select_transition_t::select(editor_animation_graph_context_t& context, u32 transition_id)
	{
		if (context.get_selected_transition_id() == transition_id)
			return true;

		const editor_command_animation_graph_select_transition_payload_t payload{
			.previous_transition_id = context.get_selected_transition_id(),
			.post_transition_id		= transition_id,
			.previous_sub_node_id	= context.get_selected_sub_node_id(),
			.post_sub_node_id		= transition_id == ANIMATION_GRAPH_DEF_NULL_ID ? context.get_selected_sub_node_id() : ANIMATION_GRAPH_DEF_NULL_ID,
		};
		const editor_command_issue_desc_t desc{
			.undo		= animation_graph_select_transition_undo,
			.redo		= animation_graph_select_transition_redo,
			.user_data	= &context,
			.debug_name = "Animation Graph Select Transition",
			.type		= editor_command_type_e::animation_graph_select_transition,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation graph select transition command");
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
			.previous_transition_id	  = context.get_selected_transition_id(),
			.post_transition_id		  = ANIMATION_GRAPH_DEF_NULL_ID,
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
}
