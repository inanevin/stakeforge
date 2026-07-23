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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>

namespace sfg
{
	class editor_animation_graph_context_t;

	struct editor_command_animation_graph_node_payload_t
	{
		chunk_handle32_t node_stream		= {};
		u32				 node_id			= ANIMATION_GRAPH_DEF_NULL_ID;
		u32				 node_index			= 0;
		u32				 previous_selection = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct editor_command_animation_graph_select_node_payload_t
	{
		u32 previous_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct editor_command_animation_graph_designate_node_payload_t
	{
		u32 previous_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct editor_command_animation_graph_connect_nodes_payload_t
	{
		u32 source_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 previous_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	enum class editor_animation_graph_change_e : u8
	{
		graph_properties,
		parameter_list,
		node_created,
		node_deleted,
		node_moved,
		node_properties,
		graph_connection,
		state_created,
		state_deleted,
		state_moved,
		state_properties,
		transition_created,
		transition_deleted,
		transition_properties,
	};

	struct editor_animation_graph_change_t
	{
		u32								object_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32								parent_id = ANIMATION_GRAPH_DEF_NULL_ID;
		editor_animation_graph_change_e type	  = editor_animation_graph_change_e::graph_properties;
	};

	struct editor_animation_graph_navigation_state_t
	{
		vec2f_t view_offset			  = vec2f_t::zero;
		u32		state_machine_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32		selection_parent_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		u32		selection_object_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		f32		zoom				  = 1.0f;
		u8		mode				  = 0;
		u8		selection_type		  = 0;
	};

	class editor_command_animation_graph_add_node_t final
	{
	public:
		editor_command_animation_graph_add_node_t()															   = delete;
		~editor_command_animation_graph_add_node_t()														   = delete;
		editor_command_animation_graph_add_node_t(const editor_command_animation_graph_add_node_t&)			   = delete;
		editor_command_animation_graph_add_node_t& operator=(const editor_command_animation_graph_add_node_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool add(editor_animation_graph_context_t& context, animation_graph_node_type_e type, const vec2f_t& editor_position, const char* name);
	};

	class editor_command_animation_graph_delete_node_t final
	{
	public:
		editor_command_animation_graph_delete_node_t()																 = delete;
		~editor_command_animation_graph_delete_node_t()																 = delete;
		editor_command_animation_graph_delete_node_t(const editor_command_animation_graph_delete_node_t&)			 = delete;
		editor_command_animation_graph_delete_node_t& operator=(const editor_command_animation_graph_delete_node_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool remove(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_duplicate_node_t final
	{
	public:
		editor_command_animation_graph_duplicate_node_t()																   = delete;
		~editor_command_animation_graph_duplicate_node_t()																   = delete;
		editor_command_animation_graph_duplicate_node_t(const editor_command_animation_graph_duplicate_node_t&)			   = delete;
		editor_command_animation_graph_duplicate_node_t& operator=(const editor_command_animation_graph_duplicate_node_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool duplicate(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_select_node_t final
	{
	public:
		editor_command_animation_graph_select_node_t()																 = delete;
		~editor_command_animation_graph_select_node_t()																 = delete;
		editor_command_animation_graph_select_node_t(const editor_command_animation_graph_select_node_t&)			 = delete;
		editor_command_animation_graph_select_node_t& operator=(const editor_command_animation_graph_select_node_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool select(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_make_entry_t final
	{
	public:
		editor_command_animation_graph_make_entry_t()															   = delete;
		~editor_command_animation_graph_make_entry_t()															   = delete;
		editor_command_animation_graph_make_entry_t(const editor_command_animation_graph_make_entry_t&)			   = delete;
		editor_command_animation_graph_make_entry_t& operator=(const editor_command_animation_graph_make_entry_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool make(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_make_exit_t final
	{
	public:
		editor_command_animation_graph_make_exit_t()															 = delete;
		~editor_command_animation_graph_make_exit_t()															 = delete;
		editor_command_animation_graph_make_exit_t(const editor_command_animation_graph_make_exit_t&)			 = delete;
		editor_command_animation_graph_make_exit_t& operator=(const editor_command_animation_graph_make_exit_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool make(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_connect_nodes_t final
	{
	public:
		editor_command_animation_graph_connect_nodes_t()																 = delete;
		~editor_command_animation_graph_connect_nodes_t()																 = delete;
		editor_command_animation_graph_connect_nodes_t(const editor_command_animation_graph_connect_nodes_t&)			 = delete;
		editor_command_animation_graph_connect_nodes_t& operator=(const editor_command_animation_graph_connect_nodes_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool connect(editor_animation_graph_context_t& context, u32 source_node_id, u32 target_node_id);
	};

	class editor_command_animation_graph_edit_t final
	{
	public:
		editor_command_animation_graph_edit_t()														   = delete;
		~editor_command_animation_graph_edit_t()													   = delete;
		editor_command_animation_graph_edit_t(const editor_command_animation_graph_edit_t&)			   = delete;
		editor_command_animation_graph_edit_t& operator=(const editor_command_animation_graph_edit_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool edit(sid_t graph_id, const animation_graph_def_t& previous, const animation_graph_def_t& post, const editor_animation_graph_change_t& change);
	};

	class editor_command_animation_graph_navigation_t final
	{
	public:
		editor_command_animation_graph_navigation_t()															   = delete;
		~editor_command_animation_graph_navigation_t()															   = delete;
		editor_command_animation_graph_navigation_t(const editor_command_animation_graph_navigation_t&)			   = delete;
		editor_command_animation_graph_navigation_t& operator=(const editor_command_animation_graph_navigation_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool edit(sid_t graph_id, const editor_animation_graph_navigation_state_t& previous, const editor_animation_graph_navigation_state_t& post);
	};
}
