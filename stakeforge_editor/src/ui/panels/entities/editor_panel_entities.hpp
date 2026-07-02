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
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "editor_selection_controller.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg::ui
{
	class input_router_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_command_system_t;
	class world_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;

	class editor_panel_entities_t final : public editor_panel_t
	{
	public:
		editor_panel_entities_t();
		~editor_panel_entities_t() override								   = default;
		editor_panel_entities_t(const editor_panel_entities_t&)			   = delete;
		editor_panel_entities_t& operator=(const editor_panel_entities_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void refresh_entities();
		void refresh_entity_name(entity_id_t entity);

	private:
		struct entity_row_t
		{
			entity_id_t		entity		 = NULL_ENTITY_ID;
			ui::widget_id_t root		 = NULL_WIDGET;
			ui::widget_id_t icon		 = NULL_WIDGET;
			ui::widget_id_t icon_text	 = NULL_WIDGET;
			ui::widget_id_t label		 = NULL_WIDGET;
			u16				depth		 = 0;
			bool			has_children = false;
		};

		struct entity_desc_t
		{
			const char* name		 = nullptr;
			entity_id_t id			 = NULL_ENTITY_ID;
			entity_id_t parent		 = NULL_ENTITY_ID;
			u16			depth		 = 0;
			bool		has_children = false;
		};

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void		  collect_entities();
		void		  append_entity_desc(const world_t& world, const ecs_component_table_t& hierarchy_table, const ecs_component_table_t& name_table, entity_id_t id, u16 depth);
		entity_row_t& get_or_create_entity_row(size_t index);
		void		  update_entity_row(entity_row_t& row, const entity_desc_t& entity, bool is_folded);
		void		  update_entity_row_background(const entity_row_t& row);
		void		  set_entity_row_visible(const entity_row_t& row, bool visible);
		void		  set_focus_state(bool focused);
		bool		  can_mutate_ui_topology() const;
		void		  request_refresh_entities();
		void		  flush_pending_ui_mutations();
		void		  select_entity_row(entity_id_t entity, bool range_select, bool incremental_select);
		void		  select_all_visible_entities();
		void		  append_selected_root_entities(frame_vector_t<entity_id_t>& out_entities) const;
		void		  collect_payload_entities(entity_id_t entity);
		void		  prune_entity_selection();
		void		  toggle_entity_fold(entity_id_t entity);
		void		  create_entity(entity_id_t parent);
		void		  start_entity_payload(entity_id_t entity);
		bool		  reparent_payload_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent);
		void		  duplicate_selected_entities();
		void		  destroy_selected_entities();
		void		  open_empty_action_menu(const vec2f_t& pos);
		void		  open_entity_action_menu(const vec2f_t& pos, entity_id_t entity);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool				 is_entity_expanded(entity_id_t entity) const;
		bool				 is_entity_selected(entity_id_t entity) const;
		bool				 is_create_enabled() const;
		bool				 has_selected_ancestor(entity_id_t entity) const;
		bool				 can_reparent_entities(const vector_t<editor_entity_payload_t>& entities, entity_id_t parent) const;
		size_t				 find_visible_entity_index(entity_id_t entity) const;
		const entity_desc_t* find_entity_desc(entity_id_t entity) const;
		const entity_row_t*	 find_row_by_widget(ui::widget_id_t id, bool match_icon) const;
		const entity_row_t*	 find_row_by_pos(const vec2f_t& pos) const;

		// -----------------------------------------------------------------------------
		// handlers
		// -----------------------------------------------------------------------------

		static void on_search_changed(void* user_data);
		static void on_empty_action_menu_command(u16 command, void* user_data);
		static void on_entity_action_menu_command(u16 command, void* user_data);
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
		static void on_entity_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_entity_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);
		static void on_selection_changed(editor_selection_controller_t& controller, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		editor_input_field_t							  _search_input				= {};
		editor_scrollbar_t								  _scrollbar				= {};
		vector_t<entity_row_t>							  _entity_rows				= {};
		vector_t<entity_desc_t>							  _entity_cache				= {};
		vector_t<entity_id_t>							  _expanded_entities		= {};
		string_t										  _search_str				= {};
		string_t										  _search_str_lower			= {};
		ui::widget_id_t									  _entity_top_row			= NULL_WIDGET;
		ui::widget_id_t									  _entity_list_area			= NULL_WIDGET;
		pool_handle_t<u32, editor_command_listener_tag_t> _command_listener			= {};
		editor_selection_listener_handle_t				  _selection_listener		= {};
		world_handle_t									  _main_world				= {};
		vector_t<editor_entity_payload_t>				  _payload_entities			= {};
		editor_entity_payload_t							  _payload_entity			= {};
		entity_id_t										  _action_menu_entity		= NULL_ENTITY_ID;
		u32												  _entity_generation		= 0;
		u32												  _visible_entity_count		= 0;
		bool											  _refresh_entities_pending = false;
		bool											  _focused					= false;
	};
}
