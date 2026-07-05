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
#include "editor_selection_controller.hpp"
#include "editor_world_metadata.hpp"
#include "editor_world_controller.hpp"
#include "assets/editor_asset_spawn.hpp"
#include "ui/panels/entities/editor_panel_entities_internal.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "commands/editor_command_prefab_spawn.hpp"
#include "commands/editor_commands_entity.hpp"
#include "commands/editor_commands_world_metadata.hpp"
#include "editor_command_system.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

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
		else if (command == entity_action_menu_create_folder)
		{
			panel.create_folder({});
		}
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

	void editor_panel_entities_t::on_folder_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (command == entity_action_menu_create_empty)
			panel.create_entity(NULL_ENTITY_ID, panel._action_menu_folder);
		else if (command == entity_action_menu_create_folder)
			panel.create_folder(panel._action_menu_folder);
		else if (command == entity_action_menu_rename_folder)
		{
			editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*panel._ui);
			SFG_ASSERT(menu != nullptr);
			menu->close_action_menu();
			panel.open_folder_rename_popup(panel._action_menu_folder);
		}
		else if (command == entity_action_menu_change_folder_color)
		{
			editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*panel._ui);
			SFG_ASSERT(menu != nullptr);
			menu->close_action_menu();
			const vec2f_t pos = panel._ui->get_input().get_mouse_position();
			panel.open_folder_color_popup(pos, panel._action_menu_folder);
		}
	}

	void editor_panel_entities_t::on_folder_rename_popup_closed(const char* value, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!panel._edit_folder.is_null())
		{
			editor_commands_world_metadata_t::rename_folder(panel._edit_folder, value);
			panel.refresh_entities();
		}
		panel._edit_folder = {};
	}

	void editor_panel_entities_t::on_folder_color_wheel_edit_begin(void*)
	{
	}

	void editor_panel_entities_t::on_folder_color_wheel_data_changed(void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!panel._edit_folder.is_null())
		{
			editor_world_metadata_t::get().set_folder_color(panel._edit_folder, panel._folder_edit_color);
			panel.refresh_entities();
		}
	}

	void editor_panel_entities_t::on_folder_color_wheel_popup_closed(void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		if (!panel._edit_folder.is_null())
		{
			const color_t color = panel._folder_edit_color;
			editor_world_metadata_t::get().set_folder_color(panel._edit_folder, panel._folder_edit_original_color);
			panel._folder_edit_color = color;
			editor_commands_world_metadata_t::change_folder_color(panel._edit_folder, panel._folder_edit_color);
			panel.refresh_entities();
		}
		panel._edit_folder = {};
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

		if (ev.key == static_cast<u16>(input_code::key_f2) && !panel._focused_folder.is_null())
		{
			panel.open_folder_rename_popup(panel._focused_folder);
			return;
		}

		if (editor_selection_controller_t::get().get_selected_entities().size == 0)
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
			const editor_outliner_row_t* const row = panel.find_row_by_widget(id, /*match_fold_icon=*/false);
			if (row != nullptr && row->type == editor_outliner_item_type_e::entity)
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
		if (!(editor_world_controller_t::get().get_main_world() == panel._main_world) || panel._entity_generation != editor_command_system_t::get().get_entity_generation())
			panel.refresh_entities();
	}

	void editor_panel_entities_t::on_entity_icon_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&		   panel = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_outliner_row_t* const row	 = panel.find_row_by_widget(id, /*match_fold_icon=*/true);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (row->type == editor_outliner_item_type_e::folder)
			{
				panel._focused_folder = row->folder_handle;
				panel.open_folder_action_menu(pos, row->folder_handle);
				return;
			}

			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else if (row->has_children && row->type == editor_outliner_item_type_e::entity)
			panel.toggle_entity_fold(row->entity);
		else if (row->has_children)
		{
			editor_world_metadata_t& metadata = editor_world_metadata_t::get();
			metadata.set_folder_folded(row->folder_handle, !metadata.get_folder(row->folder_handle).folded);
			panel.refresh_entities();
		}
	}

	void editor_panel_entities_t::on_entity_row_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_panel_entities_t&		   panel = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_outliner_row_t* const row	 = panel.find_row_by_widget(id, /*match_fold_icon=*/false);
		if (row == nullptr)
			return;

		if (btn == ui::mouse_button_e::right)
		{
			if (row->type == editor_outliner_item_type_e::folder)
			{
				panel._focused_folder = row->folder_handle;
				panel.open_folder_action_menu(pos, row->folder_handle);
				return;
			}

			if (!panel.is_entity_selected(row->entity))
				panel.select_entity_row(row->entity, false, false);
			panel.open_entity_action_menu(pos, row->entity);
		}
		else
		{
			if (row->type == editor_outliner_item_type_e::folder)
			{
				panel._focused_folder = row->folder_handle;
				return;
			}

			const bool shift_pressed = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			const bool ctrl_pressed	 = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			panel.select_entity_row(row->entity, shift_pressed, ctrl_pressed);
		}
	}

	void editor_panel_entities_t::on_entity_disable_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_entities_t&		   panel = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_outliner_row_t* const row	 = panel.find_row_by_widget(id, /*match_fold_icon=*/false);
		if (row != nullptr && row->type == editor_outliner_item_type_e::entity)
			panel.toggle_entity_disabled(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_entities_t&		   panel = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_outliner_row_t* const row	 = panel.find_row_by_widget(id, /*match_fold_icon=*/false);
		if (row == nullptr)
			return;

		if (row->type == editor_outliner_item_type_e::folder)
			panel.start_folder_payload(row->folder_handle);
		else
			panel.start_entity_payload(row->entity);
	}

	void editor_panel_entities_t::on_entity_row_double_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_entities_t&		   panel = *static_cast<editor_panel_entities_t*>(user_data);
		const editor_outliner_row_t* const row	 = panel.find_row_by_widget(id, /*match_fold_icon=*/false);
		if (row == nullptr || !row->has_children)
			return;
		if (row->type == editor_outliner_item_type_e::folder)
		{
			editor_world_metadata_t& metadata = editor_world_metadata_t::get();
			metadata.set_folder_folded(row->folder_handle, !metadata.get_folder(row->folder_handle).folded);
			panel.refresh_entities();
			return;
		}

		panel.toggle_entity_fold(row->entity);
	}

	bool editor_panel_entities_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::entity && payload.type != editor_payload_type_e::entity_multi && payload.type != editor_payload_type_e::folder && payload.type != editor_payload_type_e::asset &&
			payload.type != editor_payload_type_e::asset_multi)
			return false;
		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		const ui::layout_out_t&	 out   = panel._ui->get_tree().out(panel._entity_list_area);
		const vec2f_t&			 mouse = panel._ui->get_input().get_mouse_position();
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		const editor_outliner_row_t* const row	   = panel.find_row_by_pos(mouse);
		const entity_id_t				   parent  = row != nullptr && row->type == editor_outliner_item_type_e::entity ? row->entity : NULL_ENTITY_ID;
		const editor_world_folder_handle_t folder  = row != nullptr && row->type == editor_outliner_item_type_e::folder ? row->folder_handle : editor_world_folder_handle_t{};
		bool							   changed = false;
		if (payload.type == editor_payload_type_e::asset || payload.type == editor_payload_type_e::asset_multi)
		{
			return editor_asset_spawn_t::spawn_from_payload({
				.payload	= &payload,
				.screen_pos = mouse,
				.world		= panel._main_world,
				.parent		= parent,
			});
		}
		else if (payload.type == editor_payload_type_e::folder)
		{
			if (row != nullptr && row->type != editor_outliner_item_type_e::folder)
				return false;
			const editor_world_folder_handle_t payload_folder = *static_cast<const editor_world_folder_handle_t*>(payload.user_ptr);
			changed											  = panel.assign_payload_folder_to_folder(payload_folder, folder);
		}
		else if (payload.type == editor_payload_type_e::entity)
		{
			const editor_entity_payload_t& entity = *static_cast<const editor_entity_payload_t*>(payload.user_ptr);
			panel._payload_entities.resize(0);
			panel._payload_entities.push_back(entity);
			changed = !folder.is_null() ? panel.assign_payload_entities_to_folder(panel._payload_entities, folder) : panel.reparent_payload_entities(panel._payload_entities, parent);
			if (changed && folder.is_null())
				panel.deassign_payload_entities_from_folder(panel._payload_entities);
		}
		else
		{
			const vector_t<editor_entity_payload_t>& entities = *static_cast<const vector_t<editor_entity_payload_t>*>(payload.user_ptr);
			changed											  = !folder.is_null() ? panel.assign_payload_entities_to_folder(entities, folder) : panel.reparent_payload_entities(entities, parent);
			if (changed && folder.is_null())
				panel.deassign_payload_entities_from_folder(entities);
		}
		return changed;
	}

	void editor_panel_entities_t::on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data)
	{
		editor_panel_entities_t& panel = *static_cast<editor_panel_entities_t*>(user_data);
		switch (command.type)
		{
		case editor_command_type_e::entity_duplicate: {
			if (command.state != editor_command_state_e::done)
				return;

			const editor_command_duplicate_entity_payload_t& payload  = system.get_payload_as<editor_command_duplicate_entity_payload_t>(command);
			const entity_id_t*								 entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			const entity_id_t								 entity	  = entities[payload.count - 1];
			editor_selection_controller_t::get().issue_entity_selection({.data = &entity, .size = 1}, entity);
			break;
		}
		case editor_command_type_e::entity_info_paste: {
			const editor_command_paste_entity_info_payload_t& payload = system.get_payload_as<editor_command_paste_entity_info_payload_t>(command);
			if (!(payload.world == panel._main_world))
				return;

			if (!panel._search_str_lower.empty())
			{
				panel.refresh_entities();
				return;
			}

			const entity_id_t* entities = system.get_aux_data().get<entity_id_t>(payload.entities);
			for (u32 i = 0; i < payload.count; ++i)
				panel.refresh_entity_name(entities[i]);
			break;
		}
		case editor_command_type_e::prefab_spawn: {
			const editor_command_prefab_spawn_payload_t& payload = system.get_payload_as<editor_command_prefab_spawn_payload_t>(command);
			if (!(payload.world == panel._main_world))
				return;

			panel.refresh_entities();
			break;
		}
		default:
			break;
		}
	}

	void editor_panel_entities_t::on_selection_changed(editor_selection_controller_t&, void* user_data)
	{
		editor_panel_entities_t&		panel	 = *static_cast<editor_panel_entities_t*>(user_data);
		bool							changed	 = false;
		const span_t<const entity_id_t> selected = editor_selection_controller_t::get().get_selected_entities();
		for (size_t i = 0; i < selected.size; ++i)
			changed |= panel.reveal_entity(selected.data[i]);
		if (changed)
			panel.refresh_entities();

		for (const editor_outliner_row_t& row : editor_world_metadata_t::get().get_outliner_rows())
			panel.update_outliner_row_background(row);
	}

	void editor_panel_entities_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_panel_entities_t*>(user_data)->flush_pending_ui_mutations();
	}
}
