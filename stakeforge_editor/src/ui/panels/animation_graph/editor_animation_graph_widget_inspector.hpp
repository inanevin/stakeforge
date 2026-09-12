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

#include "ui/widgets/editor_widget_reflection.hpp"

#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_animation_graph_context_t;

	class editor_animation_graph_widget_inspector_t final
	{
	public:
		struct config_t
		{
			editor_animation_graph_context_t* context = nullptr;
		};

		editor_animation_graph_widget_inspector_t()															   = default;
		~editor_animation_graph_widget_inspector_t()														   = default;
		editor_animation_graph_widget_inspector_t(const editor_animation_graph_widget_inspector_t&)			   = delete;
		editor_animation_graph_widget_inspector_t& operator=(const editor_animation_graph_widget_inspector_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_inspector();
		void set_asset_name(const char* asset_name);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_dropdown_items();
		void refresh_inspector_immediate();
		void on_edit_begin();
		void on_edit_submitted();

		static span_t<const editor_widget_reflection_dropdown_item_t> resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data);
		static void													  on_edit_begin(void* user_data);
		static void													  on_edit_submitted(void* user_data);

	private:
		skeleton_def_t									   _skeleton				   = {};
		editor_widget_reflection_t						   _graph_reflection		   = {};
		editor_widget_reflection_t						   _asm_node_reflection		   = {};
		editor_widget_reflection_t						   _asm_state_reflection	   = {};
		editor_widget_reflection_t						   _asm_transition_reflection  = {};
		editor_widget_reflection_t						   _bone_control_reflection	   = {};
		editor_widget_reflection_t						   _ik_reflection			   = {};
		vector_t<editor_widget_reflection_dropdown_item_t> _bone_dropdown_items		   = {};
		vector_t<editor_widget_reflection_dropdown_item_t> _parameter_dropdown_items   = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _fold_states				   = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _asm_node_fold_states	   = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _asm_state_fold_states	   = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _asm_transition_fold_states = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _bone_control_fold_states   = {};
		vector_t<editor_widget_reflection_fold_state_t>	   _ik_fold_states			   = {};
		ui::ui_context*									   _ui						   = nullptr;
		config_t										   _config					   = {};
		ui::widget_id_t									   _asset_name_label		   = NULL_WIDGET;
		ui::widget_id_t									   _graph_title				   = NULL_WIDGET;
		ui::widget_id_t									   _asm_node_title			   = NULL_WIDGET;
		ui::widget_id_t									   _asm_state_title			   = NULL_WIDGET;
		ui::widget_id_t									   _asm_transition_title	   = NULL_WIDGET;
		ui::widget_id_t									   _bone_control_title		   = NULL_WIDGET;
		ui::widget_id_t									   _ik_title				   = NULL_WIDGET;
		ui::widget_id_t									   _invalid_skeleton_frame	   = NULL_WIDGET;
		ui::widget_id_t									   _invalid_skeleton_label	   = NULL_WIDGET;
		ui::widget_id_t									   _root					   = NULL_WIDGET;
		bool											   _edit_active				   = false;
	};
}
