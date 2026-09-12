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

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	struct editor_asset_deletion_listener_tag_t;

	class editor_panel_mesh_viewer_t final : public editor_panel_t
	{
	public:
		editor_panel_mesh_viewer_t();
		~editor_panel_mesh_viewer_t() override									 = default;
		editor_panel_mesh_viewer_t(const editor_panel_mesh_viewer_t&)			 = delete;
		editor_panel_mesh_viewer_t& operator=(const editor_panel_mesh_viewer_t&) = delete;

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

		void set_mesh(sid_t mesh_guid, const char* asset_name);

	private:
		void			create_preview_world();
		void			destroy_preview_world();
		void			clear_display_entity();
		void			create_display_entity();
		void			refresh_info();
		void			apply_pane_split();
		ui::widget_id_t append_property_value_row(const char* label);
		ui::widget_id_t append_value_label(ui::widget_id_t parent);

		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_widget_world_view_t								 _world_view			  = {};
		editor_split_border_t									 _split_border			  = {};
		string_t												 _asset_name			  = {};
		string_t												 _vertex_count_text		  = {};
		string_t												 _index_count_text		  = {};
		string_t												 _triangle_count_text	  = {};
		string_t												 _primitive_count_text	  = {};
		string_t												 _vertex_stride_text	  = {};
		string_t												 _is_skinned_text		  = {};
		editor_world_handle_t									 _world					  = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener = {};
		sid_t													 _mesh_guid				  = 0;
		entity_id_t												 _display_entity		  = NULL_ENTITY_ID;
		entity_id_t												 _environment_entity	  = NULL_ENTITY_ID;
		ui::widget_id_t											 _left_pane				  = NULL_WIDGET;
		ui::widget_id_t											 _right_pane			  = NULL_WIDGET;
		ui::widget_id_t											 _vertex_count_value	  = NULL_WIDGET;
		ui::widget_id_t											 _index_count_value		  = NULL_WIDGET;
		ui::widget_id_t											 _triangle_count_value	  = NULL_WIDGET;
		ui::widget_id_t											 _primitive_count_value	  = NULL_WIDGET;
		ui::widget_id_t											 _vertex_stride_value	  = NULL_WIDGET;
		ui::widget_id_t											 _is_skinned_value		  = NULL_WIDGET;
		f32														 _pane_split			  = 0.72f;
	};
}
