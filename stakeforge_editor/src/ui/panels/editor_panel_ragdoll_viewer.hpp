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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/memory/chunk_handle.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/ragdoll_def.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_ragdoll_edit_t;
	class world_t;
	struct editor_asset_deletion_listener_tag_t;

	class editor_panel_ragdoll_viewer_t final : public editor_panel_t
	{
	public:
		editor_panel_ragdoll_viewer_t();
		~editor_panel_ragdoll_viewer_t() override									   = default;
		editor_panel_ragdoll_viewer_t(const editor_panel_ragdoll_viewer_t&)			   = delete;
		editor_panel_ragdoll_viewer_t& operator=(const editor_panel_ragdoll_viewer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_ragdoll(sid_t ragdoll_guid, const char* asset_name);
		void apply_ragdoll_def(ragdoll_def_t&& ragdoll);

		inline ragdoll_def_t& get_ragdoll_def()
		{
			return _ragdoll;
		}

		inline sid_t get_ragdoll_guid() const
		{
			return _ragdoll_guid;
		}

	private:
		friend class editor_command_ragdoll_edit_t;

		void create_preview_world();
		void destroy_preview_world();
		void reload_skeleton();
		void rebuild_preview();
		void refresh_part_dropdown_names();
		void refresh_reflection();
		void apply_pane_split();
		void draw_ragdoll(world_t& world) const;

		static span_t<const editor_widget_reflection_dropdown_item_t> resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data);
		static void													  on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void													  on_edit_begin(void* user_data);
		static void													  on_edited(void* user_data);
		static void													  on_edit_submitted(void* user_data);
		static void													  on_world_tick(world_t& world, f32 delta_time, void* user_data);
		static void													  on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_widget_world_view_t								 _world_view			  = {};
		editor_widget_reflection_t								 _reflection			  = {};
		editor_split_border_t									 _split_border			  = {};
		editor_scrollbar_t										 _right_scrollbar		  = {};
		ragdoll_def_t											 _ragdoll				  = {};
		skeleton_def_t											 _skeleton				  = {};
		vector_t<mat4x3_t>										 _joint_globals			  = {};
		vector_t<editor_widget_reflection_dropdown_item_t>		 _joint_dropdown_items	  = {};
		vector_t<editor_widget_reflection_dropdown_item_t>		 _part_dropdown_items	  = {};
		vector_t<string_t>										 _part_dropdown_names	  = {};
		vector_t<editor_widget_reflection_fold_state_t>			 _fold_states			  = {};
		string_t												 _asset_name			  = {};
		editor_world_handle_t									 _world					  = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener = {};
		chunk_handle32_t										 _edit_previous_stream	  = {};
		resource_handle_t										 _preview_skeleton		  = NULL_RESOURCE_HANDLE;
		sid_t													 _ragdoll_guid			  = NULL_SID;
		ui::widget_id_t											 _left_pane				  = NULL_WIDGET;
		ui::widget_id_t											 _right_pane			  = NULL_WIDGET;
		ui::widget_id_t											 _right_content			  = NULL_WIDGET;
		u32														 _part_count			  = 0;
		f32														 _pane_split			  = 0.68f;
		bool													 _edit_active			  = false;
	};
}
