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

#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_grid.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_widget_inspector.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	struct editor_asset_deletion_listener_tag_t;

	class editor_panel_animation_graph_t final : public editor_panel_t
	{
	public:
		editor_panel_animation_graph_t();
		~editor_panel_animation_graph_t() override										 = default;
		editor_panel_animation_graph_t(const editor_panel_animation_graph_t&)			 = delete;
		editor_panel_animation_graph_t& operator=(const editor_panel_animation_graph_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& json) const override;
		void deserialize(const nlohmann::json& json) override;
		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_graph(sid_t graph_id, const char* asset_name);

	private:
		void apply_pane_split();
		void load_graph();

		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_animation_graph_context_t						 _context				  = {};
		editor_animation_graph_grid_t							 _grid					  = {};
		editor_animation_graph_widget_inspector_t				 _inspector				  = {};
		editor_scrollbar_t										 _right_scrollbar		  = {};
		editor_split_border_t									 _split_border			  = {};
		string_t												 _asset_name			  = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener = {};
		sid_t													 _graph_id				  = NULL_SID;
		ui::widget_id_t											 _left_pane				  = NULL_WIDGET;
		ui::widget_id_t											 _right_pane			  = NULL_WIDGET;
		f32														 _pane_split			  = 0.72f;
	};
}
