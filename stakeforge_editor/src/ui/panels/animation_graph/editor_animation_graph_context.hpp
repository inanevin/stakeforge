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

#include "ui/panels/animation_graph/editor_animation_graph_widget_common.hpp"

#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>

namespace sfg
{
	class editor_command_animation_graph_edit_t;
	class editor_animation_graph_grid_t;
	class editor_animation_graph_widget_inspector_t;
	class editor_command_system_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;

	class editor_animation_graph_context_t final
	{
	public:
		editor_animation_graph_context_t()													 = default;
		~editor_animation_graph_context_t()													 = default;
		editor_animation_graph_context_t(const editor_animation_graph_context_t&)			 = delete;
		editor_animation_graph_context_t& operator=(const editor_animation_graph_context_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(editor_animation_graph_grid_t* grid, editor_animation_graph_widget_inspector_t* inspector);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_display_mode(editor_animation_graph_display_mode_e mode);
		void set_asset_id(sid_t asset_id);
		void set_display_node_id(u32 node_id);
		void set_selected_node_id(u32 node_id);
		void set_selected_sub_node_id(u32 node_id);
		void set_selected_transition_id(u32 transition_id);
		u32	 acquire_node_id();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_animation_graph_display_mode_e get_display_mode() const
		{
			return _display_mode;
		}

		inline sid_t get_asset_id() const
		{
			return _asset_id;
		}

		inline u32 get_display_node_id() const
		{
			return _display_node_id;
		}

		inline u32 get_selected_node_id() const
		{
			return _selected_node_id;
		}

		inline u32 get_selected_sub_node_id() const
		{
			return _selected_sub_node_id;
		}

		inline u32 get_selected_transition_id() const
		{
			return _selected_transition_id;
		}

		inline animation_graph_def_t& get_graph()
		{
			return _graph;
		}

		inline const animation_graph_def_t& get_graph() const
		{
			return _graph;
		}

	private:
		friend class editor_command_animation_graph_edit_t;

		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);
		void		ensure_graph_node_designations();
		void		ensure_asm_state_designations();

	private:
		animation_graph_def_t							  _graph								= {};
		editor_animation_graph_grid_t*					  _grid									= nullptr;
		editor_animation_graph_widget_inspector_t*		  _inspector							= nullptr;
		pool_handle_t<u32, editor_command_listener_tag_t> _command_listener						= {};
		sid_t											  _asset_id								= NULL_SID;
		chunk_handle32_t								  _edit_previous_stream					= {};
		u32												  _display_node_id						= UINT32_MAX;
		u32												  _selected_node_id						= UINT32_MAX;
		u32												  _selected_sub_node_id					= UINT32_MAX;
		u32												  _selected_transition_id				= UINT32_MAX;
		u32												  _edit_previous_display_node_id		= UINT32_MAX;
		u32												  _edit_previous_selected_node_id		= UINT32_MAX;
		u32												  _edit_previous_selected_sub_node_id	= UINT32_MAX;
		u32												  _edit_previous_selected_transition_id = UINT32_MAX;
		u32												  _id_counter							= 1;
		editor_animation_graph_display_mode_e			  _display_mode							= editor_animation_graph_display_mode_e::display_nodes;
		editor_animation_graph_display_mode_e			  _edit_previous_mode					= editor_animation_graph_display_mode_e::display_nodes;
	};
}
