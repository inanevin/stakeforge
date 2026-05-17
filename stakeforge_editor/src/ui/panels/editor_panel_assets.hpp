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

#include "assets/editor_asset_manager.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/frame_string.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg::ui
{
	class input_router_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	enum assets_filter_flags_e : u8
	{
		assets_filter_all		 = 1 << 0,
		assets_filter_favourites = 1 << 1,
	};

	class editor_panel_assets_t final : public editor_panel_t
	{
	public:
		editor_panel_assets_t();
		~editor_panel_assets_t() override							   = default;
		editor_panel_assets_t(const editor_panel_assets_t&)			   = delete;
		editor_panel_assets_t& operator=(const editor_panel_assets_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void make_visible(bool visible) override;

	private:
		struct folder_row_t;

		void apply_pane_split();

		void		  open_filter_popup();
		void		  refresh_folder_rows();
		void		  append_folder_rows(editor_asset_node_handle_t node, u16 depth, const frame_string_t<char>& path);
		folder_row_t& get_or_create_folder_row(size_t index);
		void		  update_folder_row(folder_row_t& row, const char* name, u16 depth, u64 path_hash, bool has_children, bool is_folded);
		void		  update_folder_row_background(const folder_row_t& row);
		void		  refresh_folder_row_backgrounds();
		void		  set_folder_row_visible(const folder_row_t& row, bool visible);
		void		  select_folder_row(u64 path_hash);
		void		  toggle_folder_fold(u64 path_hash);
		bool		  folder_matches_search(const editor_asset_node_t& node) const;
		bool		  folder_subtree_matches_search(editor_asset_node_handle_t node) const;
		bool		  is_folder_folded(u64 path_hash) const;
		bool		  has_folder_child(editor_asset_node_handle_t node) const;

		static void on_filter_popup_pressed(u16 value, void* user_data);
		static void on_filter_button_pressed(bool toggled, void* user_data);
		static void on_refresh_button_pressed(bool toggled, void* user_data);
		static void on_search_changed(const char* value, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_asset_tree_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_folder_icon_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_folder_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_folder_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		struct folder_row_t
		{
			ui::widget_id_t root		 = NULL_WIDGET;
			ui::widget_id_t icon		 = NULL_WIDGET;
			ui::widget_id_t icon_text	 = NULL_WIDGET;
			ui::widget_id_t label		 = NULL_WIDGET;
			u64				path_hash	 = 0;
			u16				depth		 = 0;
			bool			has_children = false;
		};

	private:
		editor_icon_button_t   _filter_button			 = {};
		editor_icon_button_t   _refresh_button			 = {};
		editor_input_field_t   _search_input			 = {};
		editor_split_border_t  _split_border			 = {};
		editor_scrollbar_t	   _left_scrollbar			 = {};
		ui::widget_id_t		   _assets_left_pane		 = NULL_WIDGET;
		ui::widget_id_t		   _assets_left_pane_top_row = NULL_WIDGET;
		ui::widget_id_t		   _assets_left_pane_body	 = NULL_WIDGET;
		ui::widget_id_t		   _assets_body_pane		 = NULL_WIDGET;
		vector_t<folder_row_t> _folder_rows				 = {};
		vector_t<u64>		   _folded_folder_hashes	 = {};
		string_t			   _search_str				 = {};
		u64					   _selected_folder_hash	 = 0;
		u32					   _asset_tree_generation	 = 0;
		u32					   _visible_folder_row_count = 0;
		f32					   _pane_split				 = 0.3f;
		u8					   _filter_flags			 = assets_filter_all;
		bool				   _has_selected_folder		 = false;
	};
}
