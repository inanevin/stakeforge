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
#include "ui/editor_action_menu_common.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_reflect_type.hpp"
#include "ui/widgets/editor_widgets_button.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	namespace ui
	{
		class input_router_t;
		enum class mouse_button_e : u8;
	}

	class world_t;
	class editor_widget_entity_info_t;

	enum class editor_inspector_display_type_e : u8
	{
		none,
		entity,
	};

	class editor_panel_inspector_t final : public editor_panel_t
	{
	public:
		editor_panel_inspector_t();
		~editor_panel_inspector_t() override								 = default;
		editor_panel_inspector_t(const editor_panel_inspector_t&)			 = delete;
		editor_panel_inspector_t& operator=(const editor_panel_inspector_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_display_none();
		void set_display_entity(world_handle_t world, entity_id_t entity);
		void set_display_entity(world_handle_t world, span_t<const entity_id_t> entities);
		void refresh_display();

	private:
		struct component_display_t
		{
			editor_widget_fold_t*		  fold	  = nullptr;
			editor_widget_reflect_type_t* reflect = nullptr;
			sid_t						  type_id = 0;
		};

		struct component_display_state_t
		{
			vector_t<editor_widget_reflect_type_t::vector_fold_state_t> vector_fold_states = {};
			sid_t														type_id			   = 0;
			bool														folded			   = false;
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
		bool					   can_mutate_ui_topology() const;
		void					   request_refresh_display();
		void					   flush_pending_ui_mutations();
		component_display_state_t* find_component_display_state(sid_t type_id);
		void					   open_entity_info_action_menu(const vec2f_t& pos);
		void					   open_component_action_menu(const vec2f_t& pos, sid_t type_id);
		void					   open_add_component_action_menu(const vec2f_t& pos);
		void					   create_add_component_button();
		void					   copy_entity_info();
		void					   copy_component(sid_t type_id);
		bool					   is_component_removable(sid_t type_id) const;
		bool					   is_component_paste_enabled(sid_t type_id) const;

		static void on_entity_info_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_entity_info_action_menu_command(u16 command, void* user_data);
		static void on_component_settings_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_component_action_menu_command(u16 command, void* user_data);
		static void on_add_component_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_add_component_action_menu_command(u16 command, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		vector_t<component_display_state_t>		_component_states		  = {};
		vector_t<component_display_t>			_component_displays		  = {};
		vector_t<add_component_menu_category_t> _add_component_categories = {};
		vector_t<editor_action_menu_row_desc_t> _add_component_root_rows  = {};
		vector_t<sid_t>							_add_component_types	  = {};
		vector_t<entity_id_t>					_display_entities		  = {};
		editor_scrollbar_t						_scrollbar				  = {};
		editor_widget_fold_t*					_entity_info_fold		  = nullptr;
		editor_widget_entity_info_t*			_entity_info			  = nullptr;
		editor_button_t*						_add_component_button	  = nullptr;
		world_t*								_display_world			  = nullptr;
		world_handle_t							_display_world_handle	  = {};
		ui::widget_id_t							_scroll_area			  = NULL_WIDGET;
		ui::widget_id_t							_column					  = NULL_WIDGET;
		ostream_t								_copied_component_stream  = {};
		editor_entity_info_data_t				_copied_entity_info		  = {};
		sid_t									_copied_component_type	  = 0;
		sid_t									_action_menu_type_id	  = 0;
		editor_inspector_display_type_e			_display_type			  = editor_inspector_display_type_e::none;
		bool									_refresh_display_pending  = false;
		bool									_copied_entity_info_valid = false;
	};
}
