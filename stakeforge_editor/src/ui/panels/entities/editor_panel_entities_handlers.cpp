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
#include "ui/panels/entities/editor_panel_entities.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "commands/editor_commands_entity.hpp"
#include "editor_app.hpp"
#include "editor_command_system.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	void editor_panel_entities_t::on_search_changed(void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._search_str_lower		   = panel._search_str;
		string_util::to_lower(panel._search_str_lower);
		panel.refresh_entities();
	}

	void editor_panel_entities_t::on_empty_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (command == entity_action_menu_create_empty)
			panel.create_entity(NULL_ENTITY_ID);
	}

	void editor_panel_entities_t::on_entity_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (command == entity_action_menu_create_empty)
		{
			if (panel.is_create_enabled())
				panel.create_entity(panel._action_menu_entity);
		}
		else if (command == entity_action_menu_duplicate)
			panel.duplicate_selected_entities();
		else if (command == entity_action_menu_delete)
			panel.destroy_selected_entities();
	}

	void editor_panel_entities_t::on_entities_body_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (btn == ui::mouse_button_e::right && id == panel._entity_list_area)
			panel.open_empty_action_menu(pos);
	}

	void editor_panel_entities_t::on_entities_body_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel._scrollbar.scroll_y(delta);
	}

	void editor_panel_entities_t::on_entities_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press)
			return;

		editor_panel_entities_t& panel		  = *static_cast<editor_panel_entities_t*>(user_data);
		const bool				 ctrl_pressed = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		if (ev.key == static_cast<u16>(input_code::key_a) && ctrl_pressed)
		{
			panel.select_all_visible_entities();
			return;
		}

		if (editor_app_t::get().get_selection_controller().get_selected_entities().size == 0)
			return;

		if (ev.key == static_cast<u16>(input_code::key_delete))
			panel.destroy_selected_entities();
		else if (ev.key == static_cast<u16>(input_code::key_d) && ctrl_pressed)
			panel.duplicate_selected_entities();
	}

	void editor_panel_entities_t::on_entities_focus_gain(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(true);
	}

	void editor_panel_entities_t::on_entities_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_entities_t::on_entity_row_focus_gain(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		panel.set_focus_state(true);
		if (from_nav)
		{
			const entity_row_t* const row = panel.find_row_by_widget(id, /*match_icon=*/false);
			if (row != nullptr)
				panel.select_entity_row(row->entity, false, false);
		}
	}

	void editor_panel_entities_t::on_entity_row_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->set_focus_state(false);
	}

	void editor_panel_entities_t::on_entity_tree_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!(editor_app_t::get().get_main_world() == panel._main_world) || panel._entity_generation != editor_app_t::get().get_command_system().get_entity_generation())
			panel.refresh_entities();
	}

	void editor_panel_entities_t::on_entity_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else if (row->has_children)
			panel.toggle_entity_fold(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else
		{
			const bool shift_pressed = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			const bool ctrl_pressed	 = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			panel.select_entity_row(row->entity, shift_pressed, ctrl_pressed);
		}
	}

	void editor_panel_entities_t::on_entity_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr)
			return;

		panel.start_entity_payload(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_entities_t&  panel = *static_cast<editor_panel_entities_t*>(user_data);
		const entity_row_t* const row	= panel.find_row_by_widget(id, /*match_icon=*/false);
		if (row == nullptr || !row->has_children)
			return;
		panel.toggle_entity_fold(row->entity);
	}

	bool editor_panel_entities_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::entity && payload.type != editor_payload_type_e::entity_multi)
			return false;
		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		const ui::layout_out_t&	 out   = panel._ui->get_tree().out(panel._entity_list_area);
		const vec2f_t&			 mouse = panel._ui->get_input().get_mouse_position();
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		const entity_row_t* const row	  = panel.find_row_by_pos(mouse);
		const entity_id_t		  parent  = row != nullptr ? row->entity : NULL_ENTITY_ID;
		bool					  changed = false;
		if (payload.type == editor_payload_type_e::entity)
		{
			const editor_entity_payload_t& entity = *static_cast<const editor_entity_payload_t*>(payload.user_ptr);
			panel._payload_entities.resize(0);
			panel._payload_entities.push_back(entity);
			changed = panel.reparent_payload_entities(panel._payload_entities, parent);
		}
		else
		{
			const vector_t<editor_entity_payload_t>& entities = *static_cast<const vector_t<editor_entity_payload_t>*>(payload.user_ptr);
			changed											  = panel.reparent_payload_entities(entities, parent);
		}
		return changed;
	}

	void editor_panel_entities_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		if (command.type != editor_command_type_e::entity_duplicate || command.state != editor_command_state_e::done)
			return;

		editor_panel_entities_t&						 panel	  = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_command_duplicate_entity_payload_t& payload  = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
		const entity_id_t*								 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
		const entity_id_t								 entity	  = entities[payload.count - 1];
		editor_app_t::get().get_selection_controller().issue_entity_selection({.data = &entity, .size = 1}, entity);
	}

	void editor_panel_entities_t::on_selection_changed(editor_selection_controller_t&, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		for (const entity_row_t& row : panel._entity_rows)
			panel.update_entity_row_background(row);
		panel.refresh_panel_inspector();
	}

	void editor_panel_entities_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->flush_pending_ui_mutations();
	}
}
