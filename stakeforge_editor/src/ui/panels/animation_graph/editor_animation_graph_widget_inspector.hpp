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

#include "ui/widgets/editor_widget_reflection.hpp"

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
		void refresh_inspector_immediate();

		static void on_refresh_mutation(ui::ui_context& ui, void* user_data);

	private:
		editor_widget_reflection_t						_reflection				  = {};
		editor_widget_reflection_t						_asm_node_reflection	  = {};
		editor_widget_reflection_t						_bone_control_reflection  = {};
		editor_widget_reflection_t						_ik_reflection			  = {};
		vector_t<editor_widget_reflection_fold_state_t> _fold_states			  = {};
		vector_t<editor_widget_reflection_fold_state_t> _asm_node_fold_states	  = {};
		vector_t<editor_widget_reflection_fold_state_t> _bone_control_fold_states = {};
		vector_t<editor_widget_reflection_fold_state_t> _ik_fold_states			  = {};
		ui::ui_context*									_ui						  = nullptr;
		config_t										_config					  = {};
		ui::widget_id_t									_asset_name_label		  = NULL_WIDGET;
		ui::widget_id_t									_invalid_skeleton_frame	  = NULL_WIDGET;
		ui::widget_id_t									_invalid_skeleton_label	  = NULL_WIDGET;
		ui::widget_id_t									_root					  = NULL_WIDGET;
	};
}
