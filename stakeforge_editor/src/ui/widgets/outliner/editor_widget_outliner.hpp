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

#include "ui/editor_payload_controller.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_edit_context.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg::ui
{
	class input_router_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_command_system_t;
	class editor_icon_button_t;
	class editor_popup_color_wheel_t;
	class world_t;
	enum class editor_primitive_type_e : u8;
	namespace ui
	{
		class ui_context;
	}
	struct editor_command_listener_tag_t;
	struct editor_command_t;

	struct editor_outliner_row_t
	{
		ui::widget_id_t				 root			= NULL_WIDGET;
		ui::widget_id_t				 fold_icon		= NULL_WIDGET;
		ui::widget_id_t				 fold_icon_text = NULL_WIDGET;
		ui::widget_id_t				 type_icon		= NULL_WIDGET;
		ui::widget_id_t				 type_icon_text = NULL_WIDGET;
		ui::widget_id_t				 label			= NULL_WIDGET;
		editor_icon_button_t*		 disable_button = nullptr;
		entity_id_t					 entity			= NULL_ENTITY_ID;
		editor_world_folder_handle_t folder_handle	= {};
		u16							 depth			= 0;
		editor_outliner_item_type_e	 type			= editor_outliner_item_type_e::entity;
		bool						 has_children	= false;
		bool						 disabled		= false;
	};

	class editor_widget_outliner_t final
	{
	public:
		editor_widget_outliner_t()											 = default;
		~editor_widget_outliner_t()											 = default;
		editor_widget_outliner_t(const editor_widget_outliner_t&)			 = delete;
		editor_widget_outliner_t& operator=(const editor_widget_outliner_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_entities();
		void refresh_entity_name(entity_id_t entity);
		void set_edit_world(editor_world_handle_t world);
		void show_entity(entity_guid_t guid);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void				   collect_entities();
		editor_outliner_row_t& get_or_create_outliner_row(size_t index);
		void				   update_outliner_row(editor_outliner_row_t& row, const editor_outliner_item_t& item, bool is_folded);
		void				   update_outliner_row_background(const editor_outliner_row_t& row);
		void				   set_outliner_row_visible(const editor_outliner_row_t& row, bool visible);
		void				   set_focus_state(bool focused);
		bool				   can_mutate_ui_topology() const;
		void				   request_refresh_entities();
		void				   flush_pending_ui_mutations();
		void				   select_entity_row(entity_id_t entity, bool range_select, bool incremental_select);
		void				   select_all_visible_entities();
		void				   append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const;
		void				   collect_payload_entities(entity_id_t entity);
		void				   prune_entity_selection();
		bool				   reveal_entity(entity_id_t entity);
		void				   toggle_entity_fold(entity_id_t entity);
		void				   create_entity(entity_id_t parent, editor_world_folder_handle_t folder = {});
		void				   create_primitive(editor_primitive_type_e primitive, entity_id_t parent, editor_world_folder_handle_t folder = {});
		void				   create_folder(editor_world_folder_handle_t parent);
		void				   delete_folder(editor_world_folder_handle_t folder);
		void				   start_entity_payload(entity_id_t entity);
		void				   start_folder_payload(editor_world_folder_handle_t folder);
		bool				   reparent_payload_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent);
		void				   duplicate_selected_entities();
		void				   destroy_selected_entities();
		void				   open_empty_action_menu(const vec2f_t& pos);
		void				   open_entity_action_menu(const vec2f_t& pos, entity_id_t entity);
		void				   open_folder_action_menu(const vec2f_t& pos, editor_world_folder_handle_t folder);
		void				   open_folder_rename_popup(editor_world_folder_handle_t folder);
		void				   open_folder_color_popup(const vec2f_t& pos, editor_world_folder_handle_t folder);
		bool				   assign_payload_entities_to_folder(const vector_t<editor_entity_payload_t>& entities, editor_world_folder_handle_t folder);
		bool				   deassign_payload_entities_from_folder(const vector_t<editor_entity_payload_t>& entities);
		bool				   assign_payload_folder_to_folder(editor_world_folder_handle_t folder, editor_world_folder_handle_t parent);
		void				   toggle_entity_disabled(entity_id_t entity);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool						  is_entity_expanded(entity_id_t entity) const;
		bool						  is_entity_selected(entity_id_t entity) const;
		bool						  is_create_enabled() const;
		bool						  can_reparent_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent) const;
		size_t						  find_visible_entity_index(entity_id_t entity) const;
		const editor_outliner_item_t* find_outliner_item(entity_id_t entity) const;
		const editor_outliner_row_t*  find_row_by_widget(ui::widget_id_t id, bool match_fold_icon) const;
		const editor_outliner_row_t*  find_row_by_pos(const vec2f_t& pos) const;
		const editor_outliner_row_t*  find_row_by_folder(editor_world_folder_handle_t folder) const;

		// -----------------------------------------------------------------------------
		// handlers
		// -----------------------------------------------------------------------------

		static void on_search_changed(void* user_data);
		static void on_empty_action_menu_command(u16 command, void* user_data);
		static void on_entity_action_menu_command(u16 command, void* user_data);
		static void on_folder_action_menu_command(u16 command, void* user_data);
		static void on_folder_rename_popup_submitted(const char* value, void* user_data);
		static void on_folder_rename_popup_cancelled(void* user_data);
		static void on_folder_color_wheel_popup_install(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void on_folder_color_wheel_popup_uninstall(ui::ui_context& ui, void* user_data);
		static void on_folder_color_wheel_edit_begin(void* user_data);
		static void on_folder_color_wheel_data_changed(void* user_data);
		static void on_entities_body_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_entities_body_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data);
		static void on_entities_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_entities_focus_gain(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_entities_focus_lost(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_entity_row_focus_gain(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_entity_row_focus_lost(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_entity_tree_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_entity_icon_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_entity_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_entity_disable_clicked(bool toggled, void* user_data);
		static void on_entity_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_entity_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);
		static void on_selection_changed(editor_world_edit_context_t& controller, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		editor_input_field_t							  _search_input				  = {};
		editor_scrollbar_t								  _scrollbar				  = {};
		string_t										  _search_str				  = {};
		string_t										  _search_str_lower			  = {};
		vector_t<editor_outliner_row_t>					  _outliner_rows			  = {};
		pool_handle_t<u32, editor_command_listener_tag_t> _command_listener			  = {};
		editor_selection_listener_handle_t				  _selection_listener		  = {};
		editor_world_handle_t							  _edit_world				  = {};
		vector_t<editor_entity_payload_t>				  _payload_entities			  = {};
		editor_entity_payload_t							  _payload_entity			  = {};
		editor_world_folder_handle_t					  _payload_folder			  = {};
		ui::ui_context*									  _ui						  = nullptr;
		editor_popup_color_wheel_t*						  _folder_color_popup		  = nullptr;
		color_t											  _folder_edit_color		  = {};
		color_t											  _folder_edit_original_color = {};
		editor_world_folder_handle_t					  _action_menu_folder		  = {};
		editor_world_folder_handle_t					  _focused_folder			  = {};
		editor_world_folder_handle_t					  _edit_folder				  = {};
		ui::widget_id_t									  _root						  = NULL_WIDGET;
		ui::widget_id_t									  _entity_top_row			  = NULL_WIDGET;
		ui::widget_id_t									  _entity_list_area			  = NULL_WIDGET;
		entity_guid_t									  _pending_show_entity_guid	  = NULL_ENTITY_GUID;
		entity_id_t										  _action_menu_entity		  = NULL_ENTITY_ID;
		u32												  _entity_generation		  = 0;
		u32												  _visibility_generation	  = 0;
		u32												  _visible_entity_count		  = 0;
		bool											  _refresh_entities_pending	  = false;
		bool											  _focused					  = false;
	};
}
