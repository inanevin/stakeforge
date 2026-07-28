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
#include "world/editor_world_handle.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	struct editor_asset_deletion_listener_tag_t;

	struct panel_animation_data_t
	{
		resource_handle_t target_mesh	  = NULL_RESOURCE_HANDLE;
		resource_handle_t target_skeleton = NULL_RESOURCE_HANDLE;
	};

	SFG_DEFINE_TYPE_ID(panel_animation_data_t);

	struct panel_animation_data_reflection_t
	{
		panel_animation_data_reflection_t();
	};

	inline panel_animation_data_reflection_t g_reflect_panel_animation_data;

	class editor_panel_animation_t final : public editor_panel_t
	{
	public:
		editor_panel_animation_t();
		~editor_panel_animation_t() override								 = default;
		editor_panel_animation_t(const editor_panel_animation_t&)			 = delete;
		editor_panel_animation_t& operator=(const editor_panel_animation_t&) = delete;

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

		void set_animation(sid_t animation_guid, const char* asset_name);

	private:
		void create_preview_world();
		void destroy_preview_world();
		void create_display_entity();
		void clear_display_entity();
		void refresh_data_reflection();
		void apply_pane_splits();

		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_data_edit_submitted(void* user_data);
		static void on_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_left_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_widget_world_view_t								 _world_view			  = {};
		editor_widget_reflection_t								 _data_reflection		  = {};
		editor_split_border_t									 _pane_split_border		  = {};
		editor_split_border_t									 _left_pane_split_border  = {};
		inplace_vector_t<resource_handle_t, 16>					 _preview_materials		  = {};
		string_t												 _asset_name			  = {};
		panel_animation_data_t									 _data					  = {};
		editor_world_handle_t									 _world					  = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener = {};
		sid_t													 _animation_guid		  = NULL_SID;
		entity_id_t												 _display_entity		  = NULL_ENTITY_ID;
		ui::widget_id_t											 _left_pane				  = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_top			  = NULL_WIDGET;
		ui::widget_id_t											 _left_pane_bottom		  = NULL_WIDGET;
		ui::widget_id_t											 _right_pane			  = NULL_WIDGET;
		f32														 _pane_split			  = 0.72f;
		f32														 _left_pane_split		  = 0.7f;
	};
}
