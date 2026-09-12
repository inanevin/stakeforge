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

#include "commands/editor_commands_entity_info.hpp"
#include "ui/editor_action_menu_common.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include "ui/widgets/editor_widget_button.hpp"
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

		void set_display_entity(entity_id_t entity);
		void set_display_entity(span_t<const entity_id_t> entities);
		void refresh_display();
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

		struct add_component_menu_category_t
		{
			vector_t<editor_action_menu_row_desc_t> rows	 = {};
			const char*								category = nullptr;
		};

	private:
		void					   save_display_state();
		void					   clear_display();
		void					   create_entity_display();
		bool					   is_displaying_any_entity(span_t<const entity_id_t> entities) const;
		component_display_t*	   find_component_display(sid_t type_id);
		component_display_state_t* find_component_display_state(sid_t type_id);
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
		bool					   is_selection_prefab_child() const;
		void					   break_prefabs();
		void					   begin_entity_info_edit();
		void					   submit_entity_info_edit();
		void					   clear_entity_info_edit();
		bool					   serialize_component_streams(sid_t component_type, span_t<const entity_id_t> entities, vector_t<ostream_t>& out_streams) const;
		void					   begin_component_edit(sid_t component_type);
		void					   submit_component_edit(sid_t component_type);
		void					   clear_component_edit();

		static void													  on_entity_info_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void													  on_entity_info_action_menu_command(u16 command, void* user_data);
		static void													  on_entity_info_name_submitted(entity_id_t entity, void* user_data);
		static void													  on_entity_info_edit_begin(void* user_data);
		static void													  on_entity_info_edit_submitted(void* user_data);
		static void													  on_entity_info_break_prefab(void* user_data);
		static void													  on_component_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void													  on_component_action_menu_command(u16 command, void* user_data);
		static void													  on_add_component_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void													  on_add_component_action_menu_command(u16 command, void* user_data);
		static void													  on_component_edit_begin(void* user_data);
		static void													  on_component_edit_submitted(void* user_data);
		static span_t<const editor_widget_reflection_dropdown_item_t> resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data);
		static void													  on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		vector_t<string_t>								   _mask_dropdown_names			= {};
		vector_t<editor_widget_reflection_dropdown_item_t> _mask_dropdown_items			= {};
		vector_t<component_display_state_t>				   _component_states			= {};
		vector_t<editor_widget_reflection_fold_state_t>	   _field_states				= {};
		vector_t<component_display_t>					   _component_displays			= {};
		vector_t<add_component_menu_category_t>			   _add_component_categories	= {};
		vector_t<editor_action_menu_row_desc_t>			   _add_component_root_rows		= {};
		vector_t<sid_t>									   _add_component_types			= {};
		vector_t<entity_id_t>							   _display_entities			= {};
		vector_t<entity_id_t>							   _entity_info_edit_entities	= {};
		vector_t<editor_entity_info_data_t>				   _entity_info_edit_prev_infos = {};
		vector_t<entity_id_t>							   _component_edit_entities		= {};
		vector_t<ostream_t>								   _component_edit_prev_streams = {};
		editor_widget_fold_t*							   _entity_info_fold			= nullptr;
		editor_widget_entity_info_t*					   _entity_info					= nullptr;
		editor_widget_button_t*							   _add_component_button		= nullptr;
		ui::ui_context*									   _ui							= nullptr;
		ui::widget_id_t									   _root						= NULL_WIDGET;
		ui::widget_id_t									   _column						= NULL_WIDGET;
		ostream_t										   _copied_component_stream		= {};
		editor_entity_info_data_t						   _copied_entity_info			= {};
		pool_handle_t<u32, editor_command_listener_tag_t>  _command_listener			= {};
		editor_world_handle_t							   _edit_world					= {};
		sid_t											   _copied_component_type		= 0;
		sid_t											   _action_menu_type_id			= 0;
		sid_t											   _component_edit_type			= 0;
		bool											   _copied_entity_info_valid	= false;
		bool											   _entity_info_edit_active		= false;
		bool											   _component_edit_active		= false;
		bool											   _allow_prefab_blocks			= false;
	};
}
