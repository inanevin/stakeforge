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
#include "ui/panels/editor_panel_inspector.hpp"
#include "commands/editor_commands_component.hpp"
#include "editor_app.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_entity_info.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		enum inspector_component_action_menu_command_e : u16
		{
			inspector_component_action_menu_copy = 1,
			inspector_component_action_menu_reset,
			inspector_component_action_menu_remove,
		};

		editor_action_menu_row_desc_t INSPECTOR_COMPONENT_ACTION_MENU_ROWS[] = {
			{.text = "Copy", .command = inspector_component_action_menu_copy},
			{.text = "Reset", .command = inspector_component_action_menu_reset},
			{.text = "Remove", .command = inspector_component_action_menu_remove},
		};

		bool get_selected_entities_from_panel(frame_vector_t<entity_id_t>& entities, world_handle_t& world)
		{
			editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
			if (panel == nullptr)
				return false;

			editor_panel_entities_t*		entities_panel = static_cast<editor_panel_entities_t*>(panel);
			const span_t<const entity_id_t> selected	   = entities_panel->get_selected_entities();
			if (selected.size == 0)
				return false;

			world = entities_panel->get_world();
			entities.reserve(selected.size);
			for (size_t i = 0; i < selected.size; ++i)
				entities.push_back(selected.data[i]);
			return !world.is_null();
		}
	}

	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
	}

	void editor_panel_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		_scroll_area = ui.allocate_widget();
		ui.set_widget_debug_name(_scroll_area, "inspector_scroll_area");
		tree.attach(_root, _scroll_area);

		ui::layout_in_t& scroll_area_in = tree.in(_scroll_area);
		scroll_area_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		scroll_area_in.child_clip_mode	= ui::clip_mode_e::scissor_rect;
		scroll_area_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_value		= {1.0f, 1.0f};

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "inspector_column");
		tree.attach(_scroll_area, _column);

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.flags			   = ui::wf_visible;
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _scroll_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);
	}

	void editor_panel_inspector_t::uninit()
	{
		clear_display();
		_scrollbar.uninit();
		_component_states.clear();
		_display_entities.clear();
		_display_world		  = nullptr;
		_display_world_handle = {};
		_display_type		  = editor_inspector_display_type_e::none;
		_column				  = NULL_WIDGET;
		_scroll_area		  = NULL_WIDGET;
		_action_menu_type_id  = 0;

		editor_panel_t::uninit();
	}

	void editor_panel_inspector_t::set_display_none()
	{
		save_display_state();
		_display_type		  = editor_inspector_display_type_e::none;
		_display_world		  = nullptr;
		_display_world_handle = {};
		_display_entities.resize(0);
		refresh_display();
	}

	void editor_panel_inspector_t::set_display_entity(world_handle_t world, entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_display_entity(world, {.data = entities, .size = 1});
	}

	void editor_panel_inspector_t::set_display_entity(world_handle_t world, span_t<const entity_id_t> entities)
	{
		_display_type		  = editor_inspector_display_type_e::entity;
		_display_world_handle = world;
		_display_world		  = &editor_app_t::get().get_runtime().get_world(world);
		_display_entities.assign(entities.data, entities.data + entities.size);
		refresh_display();
	}

	void editor_panel_inspector_t::refresh_display()
	{
		save_display_state();
		clear_display();
		if (_display_type == editor_inspector_display_type_e::entity)
			create_entity_display();
	}

	void editor_panel_inspector_t::save_display_state()
	{
		for (const component_display_t& display : _component_displays)
		{
			component_display_state_t* state = find_component_display_state(display.type_id);
			if (state == nullptr)
			{
				_component_states.push_back({.type_id = display.type_id});
				state = &_component_states.back();
			}
			state->folded			  = display.fold->is_folded();
			state->vector_fold_states = display.reflect->get_vector_fold_states();
		}
	}

	void editor_panel_inspector_t::clear_display()
	{
		if (_entity_info != nullptr)
		{
			_entity_info->uninit();
			delete _entity_info;
			_entity_info = nullptr;
		}

		for (component_display_t& display : _component_displays)
		{
			display.reflect->uninit();
			display.fold->uninit();
			delete display.reflect;
			delete display.fold;
		}
		_component_displays.resize(0);
	}

	void editor_panel_inspector_t::create_entity_display()
	{
		if (_display_entities.empty())
			return;

		const entity_id_t first_entity = _display_entities.front();
		_entity_info				   = new editor_widget_entity_info_t();
		_entity_info->init(*_ui, _column, _display_world_handle);
		_entity_info->set_entity(*_display_world, first_entity);

		for (const world_component_table_t& component_table : _display_world->get_component_tables())
		{
			if (!ecs_t::table_has(component_table.table, first_entity))
				continue;

			const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(component_table.type_desc.type_id);
			if (reflected_type == nullptr || (reflected_type->flags & reflected_type_flags_no_ui) != 0)
				continue;

			bool common_component = true;
			for (size_t i = 1; i < _display_entities.size(); ++i)
			{
				if (!ecs_t::table_has(component_table.table, _display_entities[i]))
				{
					common_component = false;
					break;
				}
			}

			if (!common_component)
				continue;

			component_display_t display = {};
			display.fold				= new editor_widget_fold_t();
			display.reflect				= new editor_widget_reflect_type_t();
			display.type_id				= component_table.type_desc.type_id;

			component_display_state_t* state = find_component_display_state(display.type_id);
			display.fold->init(*_ui, _column, {.label = reflected_type->display_name, .folded = state != nullptr && state->folded, .settings_button = true});
			display.reflect->init(*_ui, display.fold->get_body());
			editor_reflected_edit_target_t target = {};
			target.world						  = _display_world_handle;
			target.entity						  = first_entity;
			target.type_id						  = component_table.type_desc.type_id;
			target.kind							  = editor_reflected_edit_target_kind_e::world_component;
			display.reflect->set_reflected_obj(ecs_t::table_get(component_table.table, first_entity), component_table.type_desc.type_id, target);
			if (state != nullptr)
				display.reflect->set_vector_fold_states(state->vector_fold_states);

			ui::listener_bundle_t settings_listener = {};
			settings_listener.user_data				= this;
			settings_listener.on_click				= on_component_settings_clicked;
			_ui->get_input().set_listener(display.fold->get_settings_button(), settings_listener);

			_component_displays.push_back(display);
		}
	}

	editor_panel_inspector_t::component_display_state_t* editor_panel_inspector_t::find_component_display_state(sid_t type_id)
	{
		for (component_display_state_t& state : _component_states)
		{
			if (state.type_id == type_id)
				return &state;
		}
		return nullptr;
	}

	void editor_panel_inspector_t::open_component_action_menu(const vec2f_t& pos, sid_t type_id)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_type_id							 = type_id;
		INSPECTOR_COMPONENT_ACTION_MENU_ROWS[2].disabled = !is_component_removable(type_id);

		editor_action_menu_desc_t desc = {};
		desc.rows					   = INSPECTOR_COMPONENT_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(INSPECTOR_COMPONENT_ACTION_MENU_ROWS) / sizeof(INSPECTOR_COMPONENT_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_component_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	bool editor_panel_inspector_t::is_component_removable(sid_t type_id) const
	{
		return type_id != component_transform_t::TYPE_ID && type_id != component_name_t::TYPE_ID;
	}

	void editor_panel_inspector_t::on_component_settings_clicked(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		for (const component_display_t& display : panel._component_displays)
		{
			if (display.fold->get_settings_button() == id)
			{
				panel.open_component_action_menu(pos, display.type_id);
				return;
			}
		}
	}

	void editor_panel_inspector_t::on_component_action_menu_command(u16 command, void* user_data)
	{
		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		switch (command)
		{
		case inspector_component_action_menu_copy:
			break;
		case inspector_component_action_menu_reset: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world))
				editor_commands_component_t::reset(world, entities, panel._action_menu_type_id);
			break;
		}
		case inspector_component_action_menu_remove: {
			frame_vector_t<entity_id_t> entities;
			world_handle_t				world = {};
			if (get_selected_entities_from_panel(entities, world))
				editor_commands_component_t::remove(world, entities, panel._action_menu_type_id);
			break;
		}
		default:
			break;
		}
	}
}
