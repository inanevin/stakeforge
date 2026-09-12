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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>

namespace sfg
{
	class editor_animation_graph_context_t;
	enum class editor_animation_graph_display_mode_e : u8;

	struct editor_command_animation_graph_select_transition_payload_t
	{
		u32 previous_transition_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_transition_id	   = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 previous_sub_node_id   = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_sub_node_id	   = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct editor_command_animation_graph_select_node_payload_t
	{
		u32 previous_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 previous_sub_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_node_id		 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_sub_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
	};

	struct editor_command_animation_graph_display_mode_payload_t
	{
		u32 previous_display_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_display_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 previous_sub_node_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_sub_node_id		 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 previous_transition_id	 = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 post_transition_id		 = ANIMATION_GRAPH_DEF_NULL_ID;
		u8	previous_mode			 = 0;
		u8	post_mode				 = 0;
	};

	struct editor_animation_graph_navigation_state_t
	{
		u32 display_node_id		   = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 selected_node_id	   = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 selected_sub_node_id   = ANIMATION_GRAPH_DEF_NULL_ID;
		u32 selected_transition_id = ANIMATION_GRAPH_DEF_NULL_ID;
		u8	mode				   = 0;
	};

	struct editor_command_animation_graph_edit_payload_t
	{
		editor_animation_graph_navigation_state_t previous_navigation = {};
		editor_animation_graph_navigation_state_t post_navigation	  = {};
		chunk_handle32_t						  previous_stream	  = {};
		chunk_handle32_t						  post_stream		  = {};
		bool									  graph_changed		  = false;
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

		static bool begin(editor_animation_graph_context_t& context);
		static bool submit(editor_animation_graph_context_t& context, const char* debug_name, bool notify);
		static void cancel(editor_animation_graph_context_t& context);
		static bool add_node(editor_animation_graph_context_t& context, animation_graph_node_type_e type, const vec2f_t& editor_position, const char* name);
		static bool delete_node(editor_animation_graph_context_t& context, u32 node_id);
		static bool duplicate_node(editor_animation_graph_context_t& context, u32 node_id);
		static bool add_asm_state(editor_animation_graph_context_t& context, const vec2f_t& editor_position, const char* name);
		static bool delete_asm_state(editor_animation_graph_context_t& context, u32 state_id);
		static bool duplicate_asm_state(editor_animation_graph_context_t& context, u32 state_id);
		static bool add_asm_transition(editor_animation_graph_context_t& context, u32 from_state_id, u32 to_state_id);
		static bool delete_asm_transition(editor_animation_graph_context_t& context, u32 transition_id);
		static bool make_entry(editor_animation_graph_context_t& context, u32 node_id);
		static bool make_exit(editor_animation_graph_context_t& context, u32 node_id);
		static bool make_start_state(editor_animation_graph_context_t& context, u32 state_id);
		static bool connect_nodes(editor_animation_graph_context_t& context, u32 source_node_id, u32 target_node_id);
	};

	class editor_command_animation_graph_select_transition_t final
	{
	public:
		editor_command_animation_graph_select_transition_t()																	 = delete;
		~editor_command_animation_graph_select_transition_t()																	 = delete;
		editor_command_animation_graph_select_transition_t(const editor_command_animation_graph_select_transition_t&)			 = delete;
		editor_command_animation_graph_select_transition_t& operator=(const editor_command_animation_graph_select_transition_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool select(editor_animation_graph_context_t& context, u32 transition_id);
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
		static bool select_sub_node(editor_animation_graph_context_t& context, u32 node_id);
	};

	class editor_command_animation_graph_set_display_mode_t final
	{
	public:
		editor_command_animation_graph_set_display_mode_t()																	   = delete;
		~editor_command_animation_graph_set_display_mode_t()																   = delete;
		editor_command_animation_graph_set_display_mode_t(const editor_command_animation_graph_set_display_mode_t&)			   = delete;
		editor_command_animation_graph_set_display_mode_t& operator=(const editor_command_animation_graph_set_display_mode_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool set(editor_animation_graph_context_t& context, editor_animation_graph_display_mode_e mode, u32 display_node_id);
	};
}
