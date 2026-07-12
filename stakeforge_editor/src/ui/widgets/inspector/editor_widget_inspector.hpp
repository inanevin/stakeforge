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

#include "commands/editor_commands_entity_info.hpp"
#include "world/editor_world_edit_context.hpp"
#include "ui/editor_action_menu_common.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	namespace ui
	{
		class input_router_t;
		class ui_context;
		enum class mouse_button_e : u8;
	}

	class editor_command_system_t;
	class world_t;
	class editor_widget_entity_info_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;

	enum class editor_inspector_display_type_e : u8
	{
		none,
		entity,
	};

	struct editor_widget_inspector_config_t
	{
		bool allow_prefab_blocks = false;
	};

	class editor_widget_inspector_t final
	{
	public:
		editor_widget_inspector_t()											   = default;
		~editor_widget_inspector_t()										   = default;
		editor_widget_inspector_t(const editor_widget_inspector_t&)			   = delete;
		editor_widget_inspector_t& operator=(const editor_widget_inspector_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_inspector_config_t& config = {});
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_display_none();
		void set_display_entity(entity_id_t entity);
		void set_display_entity(span_t<const entity_id_t> entities);
		void refresh_display();
		void refresh_from_selection();
		void refresh_component_reflection(sid_t component_type);
		void set_edit_world(editor_world_handle_t world);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		struct component_edit_callback_data_t
		{
			editor_widget_inspector_t* panel		  = nullptr;
			sid_t					   component_type = 0;
		};

		struct component_display_t
		{
			vector_t<void*>					objects		   = {};
			editor_widget_reflection_t*		reflect		   = nullptr;
			editor_widget_fold_t*			fold		   = nullptr;
			component_edit_callback_data_t* edit_user_data = nullptr;
			sid_t							type_id		   = 0;
		};

		struct component_display_state_t
		{
			sid_t type_id = 0;
			bool  folded  = false;
		};

		struct entity_scroll_state_t
		{
			entity_id_t entity	 = {};
			f32			scroll_y = 0.0f;
		};

		struct add_component_menu_category_t
		{
			vector_t<editor_action_menu_row_desc_t> rows	 = {};
			const char*								category = nullptr;
		};

	private:
		void					   save_display_state();
		void					   save_scroll_state();
		void					   restore_scroll_state();
		void					   clear_display();
		void					   create_entity_display();
		bool					   can_mutate_ui_topology() const;
		void					   request_refresh_display();
		void					   request_refresh_component_reflection(sid_t component_type);
		void					   flush_pending_ui_mutations();
		void					   apply_pending_scroll_restore();
		bool					   is_displaying_any_entity(span_t<const entity_id_t> entities) const;
		component_display_t*	   find_component_display(sid_t type_id);
		component_display_state_t* find_component_display_state(sid_t type_id);
		entity_scroll_state_t*	   find_entity_scroll_state(entity_id_t entity);
		void					   open_entity_info_action_menu(const vec2f_t& pos);
		void					   open_component_action_menu(const vec2f_t& pos, sid_t type_id);
		void					   open_add_component_action_menu(const vec2f_t& pos);
		void					   create_add_component_button();
		void					   copy_entity_info();
		void					   copy_component(sid_t type_id);
		bool					   is_component_removable(sid_t type_id) const;
		bool					   is_component_paste_enabled(sid_t type_id) const;
		bool					   read_entity_infos(span_t<const entity_id_t> entities, vector_t<editor_entity_info_data_t>& out_infos) const;
		bool					   is_selection_prefab_referenced() const;
		void					   break_prefabs();
		void					   begin_entity_info_edit();
		void					   submit_entity_info_edit();
		void					   clear_entity_info_edit();
		bool					   serialize_component_streams(sid_t component_type, span_t<const entity_id_t> entities, vector_t<ostream_t>& out_streams) const;
		void					   begin_component_edit(sid_t component_type);
		void					   submit_component_edit(sid_t component_type);
		void					   clear_component_edit();

		static void on_entity_info_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_entity_info_action_menu_command(u16 command, void* user_data);
		static void on_entity_info_name_submitted(entity_id_t entity, void* user_data);
		static void on_entity_info_edit_begin(void* user_data);
		static void on_entity_info_edit_submitted(void* user_data);
		static void on_entity_info_break_prefab(void* user_data);
		static void on_component_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_component_action_menu_command(u16 command, void* user_data);
		static void on_add_component_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_add_component_action_menu_command(u16 command, void* user_data);
		static void on_component_edit_begin(void* user_data);
		static void on_component_edit_submitted(void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);
		static void on_selection_changed(editor_world_edit_context_t& controller, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);
		static void on_scroll_restore_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		vector_t<component_display_state_t>				  _component_states			   = {};
		vector_t<editor_widget_reflection_fold_state_t>	  _field_states				   = {};
		vector_t<entity_scroll_state_t>					  _entity_scroll_states		   = {};
		vector_t<component_display_t>					  _component_displays		   = {};
		vector_t<add_component_menu_category_t>			  _add_component_categories	   = {};
		vector_t<editor_action_menu_row_desc_t>			  _add_component_root_rows	   = {};
		vector_t<sid_t>									  _add_component_types		   = {};
		vector_t<entity_id_t>							  _display_entities			   = {};
		vector_t<entity_id_t>							  _entity_info_edit_entities   = {};
		vector_t<editor_entity_info_data_t>				  _entity_info_edit_prev_infos = {};
		vector_t<entity_id_t>							  _component_edit_entities	   = {};
		vector_t<ostream_t>								  _component_edit_prev_streams = {};
		editor_scrollbar_t								  _scrollbar				   = {};
		editor_widget_fold_t*							  _entity_info_fold			   = nullptr;
		editor_widget_entity_info_t*					  _entity_info				   = nullptr;
		editor_widget_button_t*							  _add_component_button		   = nullptr;
		ui::ui_context*									  _ui						   = nullptr;
		ui::widget_id_t									  _root						   = NULL_WIDGET;
		ui::widget_id_t									  _scroll_area				   = NULL_WIDGET;
		ui::widget_id_t									  _column					   = NULL_WIDGET;
		ostream_t										  _copied_component_stream	   = {};
		editor_entity_info_data_t						  _copied_entity_info		   = {};
		pool_handle_t<u32, editor_command_listener_tag_t> _command_listener			   = {};
		editor_selection_listener_handle_t				  _selection_listener		   = {};
		editor_world_handle_t							  _edit_world				   = {};
		sid_t											  _copied_component_type	   = 0;
		sid_t											  _action_menu_type_id		   = 0;
		sid_t											  _pending_component_type	   = 0;
		sid_t											  _component_edit_type		   = 0;
		editor_inspector_display_type_e					  _display_type				   = editor_inspector_display_type_e::none;
		f32												  _pending_scroll_y			   = 0.0f;
		bool											  _refresh_display_pending	   = false;
		bool											  _refresh_component_pending   = false;
		bool											  _scroll_restore_pending	   = false;
		bool											  _skip_scroll_state_save	   = false;
		bool											  _copied_entity_info_valid	   = false;
		bool											  _entity_info_edit_active	   = false;
		bool											  _component_edit_active	   = false;
		bool											  _allow_prefab_blocks		   = false;
	};
}
