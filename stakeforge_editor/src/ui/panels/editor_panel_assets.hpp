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

#include "assets/editor_asset_importer.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/editor_modal_cook_options.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/frame_string.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace sfg::ui
{
	class input_router_t;
	struct key_event_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_payload_t;

	class editor_panel_assets_t final : public editor_panel_t
	{
	public:
		editor_panel_assets_t();
		~editor_panel_assets_t() override							   = default;
		editor_panel_assets_t(const editor_panel_assets_t&)			   = delete;
		editor_panel_assets_t& operator=(const editor_panel_assets_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;
		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void make_visible(bool visible) override;

	private:
		struct folder_row_t
		{
			editor_asset_node_handle_t node			= {};
			u64						   path_hash	= 0;
			ui::widget_id_t			   root			= NULL_WIDGET;
			ui::widget_id_t			   icon			= NULL_WIDGET;
			ui::widget_id_t			   icon_text	= NULL_WIDGET;
			ui::widget_id_t			   star_text	= NULL_WIDGET;
			ui::widget_id_t			   label		= NULL_WIDGET;
			u16						   depth		= 0;
			bool					   has_children = false;
			bool					   is_favourite = false;
		};

		struct asset_grid_item_t
		{
			editor_asset_node_handle_t node			   = {};
			ui::widget_id_t			   root			   = NULL_WIDGET;
			ui::widget_id_t			   thumbnail_frame = NULL_WIDGET;
			ui::widget_id_t			   info_frame	   = NULL_WIDGET;
			ui::widget_id_t			   color_frame	   = NULL_WIDGET;
			ui::widget_id_t			   label		   = NULL_WIDGET;
			ui::widget_id_t			   type_label	   = NULL_WIDGET;
			ui::widget_id_t			   status_text	   = NULL_WIDGET;
			ui::widget_id_t			   star_text	   = NULL_WIDGET;
		};

		enum class asset_item_style_e : u8
		{
			grid,
			list,
		};

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void apply_pane_split();
		void open_filter_popup();
		void open_action_menu(const vec2f_t& pos);
		void open_asset_action_menu(const vec2f_t& pos);
		void import_assets(const vector_t<string_t>& paths);
		void refresh_folder_rows();
		bool append_folder_rows(editor_asset_node_handle_t node, u16 depth, frame_string_t<char>& current_path);
		void refresh_asset_grid(bool force);
		void clear_asset_grid();
		void append_asset_grid_item(ui::widget_id_t row, editor_asset_node_handle_t node, const vec2f_t& item_size);
		void append_asset_list_item(editor_asset_node_handle_t node);
		void update_current_directory_label();
		void update_import_button_state();

		folder_row_t& get_or_create_folder_row(size_t index);
		void		  update_folder_row(folder_row_t& row, editor_asset_node_handle_t node, const char* name, u16 depth, u64 path_hash, bool has_children, bool is_folded, bool is_favourite);
		void		  update_folder_row_background(const folder_row_t& row);
		void		  set_folder_row_visible(const folder_row_t& row, bool visible);

		void select_folder_row(editor_asset_node_handle_t node, u64 path_hash);
		void select_asset_grid_item(editor_asset_node_handle_t node);
		void start_folder_payload(editor_asset_node_handle_t node);
		void start_asset_item_payload(editor_asset_node_handle_t node);
		void clear_asset_grid_selection();
		void refresh_asset_grid_item_backgrounds();
		void refresh_asset_favourite_icons();
		void toggle_folder_fold(u64 path_hash);
		void toggle_folder_favourite(u64 path_hash);
		void toggle_asset_favourite(sid_t guid);
		void collect_pending_import_options(const vector_t<string_t>& paths);
		void submit_pending_import();
		void clear_pending_import();
		void delete_folder();
		void duplicate_folder();
		void delete_asset();
		void duplicate_asset();
		void fix_asset_integrity();
		void open_create_folder_popup();
		void create_folder(const char* name);
		void open_create_asset_popup(u16 command);
		void create_asset_item(u16 command, const char* name);
		void open_rename_popup();
		void rename_folder(const char* name);
		void open_asset_rename_popup();
		void rename_asset_item(const char* name);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		string_t				 get_action_menu_target_folder_path() const;
		u64						 get_folder_hash_after_rename(editor_asset_node_handle_t node, const string_t& name) const;
		const char*				 get_selected_folder_path() const;
		sid_t					 get_asset_guid(editor_asset_node_handle_t node) const;
		bool					 is_asset_favourite(sid_t guid) const;
		bool					 has_favourite_asset_descendant(editor_asset_node_handle_t node) const;
		const folder_row_t*		 find_row_by_hash(u64 path_hash) const;
		const folder_row_t*		 find_row_by_widget(ui::widget_id_t id, bool match_icon) const;
		const folder_row_t*		 find_row_by_pos(const vec2f_t& pos) const;
		const asset_grid_item_t* find_asset_grid_item_by_widget(ui::widget_id_t id) const;

		// -----------------------------------------------------------------------------
		// handlers
		// -----------------------------------------------------------------------------

		static void on_filter_popup_pressed(u16 value, void* user_data);
		static void on_filter_button_pressed(bool toggled, void* user_data);
		static void on_import_button_pressed(bool toggled, void* user_data);
		static void on_refresh_button_pressed(bool toggled, void* user_data);
		static void on_action_menu_command(u16 command, void* user_data);
		static void on_asset_action_menu_command(u16 command, void* user_data);
		static void on_action_menu_closed(void* user_data);
		static void on_create_folder_popup_closed(const char* value, void* user_data);
		static void on_create_asset_popup_closed(const char* value, void* user_data);
		static void on_rename_popup_closed(const char* value, void* user_data);
		static void on_asset_rename_popup_closed(const char* value, void* user_data);
		static void on_cook_options_imported(void* user_data);
		static void on_cook_options_cancelled(void* user_data);
		static void on_search_changed(const char* value, void* user_data);
		static void on_asset_search_changed(const char* value, void* user_data);
		static void on_show_file_assets_pressed(bool toggled, void* user_data);
		static void on_asset_favourites_only_pressed(bool toggled, void* user_data);
		static u16	get_selected_item_style(void* user_data);
		static void on_item_style_pressed(u16 value, void* user_data);
		static void on_asset_tree_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_assets_body_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_assets_body_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_asset_tree_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_asset_grid_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_asset_grid_item_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_asset_grid_item_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_folder_icon_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_folder_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_folder_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_folder_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		editor_icon_button_t					_filter_button					 = {};
		editor_icon_button_t					_import_button					 = {};
		editor_icon_button_t					_refresh_button					 = {};
		editor_icon_button_t					_show_file_assets_button		 = {};
		editor_icon_button_t					_asset_favourites_only_button	 = {};
		editor_input_field_t					_search_input					 = {};
		editor_input_field_t					_asset_search_input				 = {};
		editor_dropdown_t						_item_style_dropdown			 = {};
		editor_split_border_t					_split_border					 = {};
		editor_scrollbar_t						_left_scrollbar					 = {};
		editor_scrollbar_t						_right_scrollbar				 = {};
		vector_t<folder_row_t>					_folder_rows					 = {};
		vector_t<ui::widget_id_t>				_asset_grid_rows				 = {};
		vector_t<asset_grid_item_t>				_asset_grid_items				 = {};
		vector_t<u64>							_expanded_folder_hashes			 = {};
		vector_t<u64>							_favourite_folder_hashes		 = {};
		vector_t<sid_t>							_favourite_asset_guids			 = {};
		vector_t<string_t>						_pending_import_paths			 = {};
		vector_t<editor_asset_import_options_t> _pending_import_options			 = {};
		editor_modal_cook_options_t				_cook_options_modal				 = {};
		string_t								_search_str						 = {};
		string_t								_search_str_lower				 = {};
		string_t								_asset_search_str				 = {};
		string_t								_asset_search_str_lower			 = {};
		vec2f_t									_action_menu_pos				 = {};
		vec2f_t									_asset_grid_body_size			 = {};
		u64										_selected_folder_hash			 = UINT64_MAX;
		u64										_asset_grid_folder_hash			 = UINT64_MAX;
		editor_asset_node_handle_t				_selected_folder_node			 = {};
		editor_asset_node_handle_t				_selected_asset_node			 = {};
		editor_asset_node_handle_t				_payload_asset_node				 = {};
		editor_asset_node_handle_t				_payload_folder_node			 = {};
		ui::widget_id_t							_assets_left_pane				 = NULL_WIDGET;
		ui::widget_id_t							_assets_left_pane_top_row		 = NULL_WIDGET;
		ui::widget_id_t							_assets_left_pane_body			 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane				 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_top			 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_divider		 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_bottom		 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_bottom_divider = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_path			 = NULL_WIDGET;
		ui::widget_id_t							_assets_body_pane_controls		 = NULL_WIDGET;
		u32										_asset_tree_generation			 = 0;
		u32										_asset_grid_generation			 = 0;
		u32										_visible_folder_row_count		 = 0;
		u16										_create_asset_popup_command		 = 0;
		f32										_pane_split						 = 0.3f;
		asset_item_style_e						_asset_item_style				 = asset_item_style_e::grid;
		bool									_favourites_only				 = false;
		bool									_show_file_assets				 = false;
		bool									_asset_favourites_only			 = false;
		bool									_create_folder_popup_pending	 = false;
		bool									_rename_popup_pending			 = false;
		bool									_asset_rename_popup_pending		 = false;
		bool									_asset_grid_body_size_valid		 = false;
		bool									_asset_grid_rebuild_pending		 = false;
	};
}
