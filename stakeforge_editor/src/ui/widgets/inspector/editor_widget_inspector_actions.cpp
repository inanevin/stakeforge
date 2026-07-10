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
#include "ui/widgets/inspector/editor_widget_inspector.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/reflection/reflection_registry.hpp>
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
			inspector_component_action_menu_paste,
			inspector_component_action_menu_reset,
			inspector_component_action_menu_remove,
		};

		enum inspector_entity_info_action_menu_command_e : u16
		{
			inspector_entity_info_action_menu_copy = 1,
			inspector_entity_info_action_menu_paste,
		};

		editor_action_menu_row_desc_t INSPECTOR_COMPONENT_ACTION_MENU_ROWS[] = {
			{.text = "Copy", .command = inspector_component_action_menu_copy},
			{.text = "Paste", .command = inspector_component_action_menu_paste},
			{.text = "Reset", .command = inspector_component_action_menu_reset},
			{.text = "Remove", .command = inspector_component_action_menu_remove},
		};

		editor_action_menu_row_desc_t INSPECTOR_ENTITY_INFO_ACTION_MENU_ROWS[] = {
			{.text = "Copy", .command = inspector_entity_info_action_menu_copy},
			{.text = "Paste", .command = inspector_entity_info_action_menu_paste},
		};

		bool get_selected_entities_from_panel(editor_world_handle_t edit_world, frame_vector_t<entity_id_t>& entities, editor_world_handle_t& world)
		{
			editor_world_edit_context_t&	controller = editor_world_controller_t::get().get_editor_world(edit_world)->get_edit_context();
			const span_t<const entity_id_t> selected   = controller.get_selected_entities();
			if (selected.size == 0)
				return false;

			world = controller.get_world();
			entities.reserve(selected.size);
			for (size_t i = 0; i < selected.size; ++i)
				entities.push_back(selected.data[i]);
			return !world.is_null();
		}
	}
	void editor_widget_inspector_t::open_entity_info_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		INSPECTOR_ENTITY_INFO_ACTION_MENU_ROWS[1].disabled = !_copied_entity_info_valid;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = INSPECTOR_ENTITY_INFO_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(INSPECTOR_ENTITY_INFO_ACTION_MENU_ROWS) / sizeof(INSPECTOR_ENTITY_INFO_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_entity_info_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_widget_inspector_t::open_component_action_menu(const vec2f_t& pos, sid_t type_id)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);

		_action_menu_type_id							 = type_id;
		INSPECTOR_COMPONENT_ACTION_MENU_ROWS[1].disabled = !is_component_paste_enabled(type_id);
		INSPECTOR_COMPONENT_ACTION_MENU_ROWS[3].disabled = !is_component_removable(type_id);

		editor_action_menu_desc_t desc = {};
		desc.rows					   = INSPECTOR_COMPONENT_ACTION_MENU_ROWS;
		desc.row_count				   = static_cast<u16>(sizeof(INSPECTOR_COMPONENT_ACTION_MENU_ROWS) / sizeof(INSPECTOR_COMPONENT_ACTION_MENU_ROWS[0]));
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_component_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_widget_inspector_t::open_add_component_action_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu = editor_action_menu_controller_t::find(*_ui);
		SFG_ASSERT(menu != nullptr);
		SFG_ASSERT(!_edit_world.is_null());

		_add_component_categories.resize(0);
		_add_component_root_rows.resize(0);
		_add_component_types.resize(0);

		world_t&							   world			= editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		const vector_t<ecs_component_table_t>& component_tables = world.get_component_tables();
		_add_component_categories.reserve(component_tables.size());
		_add_component_types.reserve(component_tables.size());

		for (const ecs_component_table_t& component_table : component_tables)
		{
			const reflected_type_t* reflected_type = reflection_registry_t::get().find_type(component_table.type_desc.type_id);
			if (reflected_type == nullptr || reflected_type->flags.is_set(reflected_type_flag_no_ui))
				continue;

			const char*					   category			= "Component";
			add_component_menu_category_t* category_storage = nullptr;
			for (add_component_menu_category_t& candidate : _add_component_categories)
			{
				if (std::strcmp(candidate.category, category) == 0)
				{
					category_storage = &candidate;
					break;
				}
			}
			if (category_storage == nullptr)
			{
				_add_component_categories.push_back({.category = category});
				category_storage = &_add_component_categories.back();
			}

			_add_component_types.push_back(component_table.type_desc.type_id);
			category_storage->rows.push_back({.text = reflected_type->display_name != nullptr ? reflected_type->display_name : reflected_type->name, .command = static_cast<u16>(_add_component_types.size())});
		}

		_add_component_root_rows.reserve(_add_component_categories.size());
		for (add_component_menu_category_t& category : _add_component_categories)
		{
			if (category.rows.empty())
				continue;
			_add_component_root_rows.push_back({.text = category.category, .children = category.rows.data(), .child_count = static_cast<u16>(category.rows.size())});
		}
		if (_add_component_root_rows.empty())
			return;

		editor_action_menu_desc_t desc = {};
		desc.rows					   = _add_component_root_rows.data();
		desc.row_count				   = static_cast<u16>(_add_component_root_rows.size());
		desc.pos					   = pos;
		desc.style					   = make_default_action_menu_style(editor_theme_t::get());
		desc.command_fn				   = on_add_component_action_menu_command;
		desc.command_user_data		   = this;
		menu->request_action_menu(desc);
	}

	void editor_widget_inspector_t::create_add_component_button()
	{
		const editor_theme_t& theme	  = editor_theme_t::get();
		_add_component_button		  = new editor_button_t();
		editor_button_config_t config = {};
		config.text					  = "Add Component";
		config.width				  = {.mode = editor_widget_width_e::fixed, .value = theme.item_width * 1.5f};
		_add_component_button->init(*_ui, _column, config);

		ui::layout_in_t& button_in									  = _ui->get_tree().in(_add_component_button->get_root());
		button_in.pos_mode_x										  = ui::pos_mode_e::relative_in_parent;
		button_in.pos_value.x										  = 0.5f;
		button_in.anchor_x											  = ui::anchor_e::center;
		_ui->get_tree().draw_order(_add_component_button->get_root()) = _ui->get_tree().draw_order_const(_column) + 1;

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_add_component_clicked;
		_ui->get_input().set_listener(_add_component_button->get_root(), listener);
	}

	bool editor_widget_inspector_t::is_component_removable(sid_t type_id) const
	{
		return type_id != type_id_t<component_transform_t>::value && type_id != type_id_t<component_name_t>::value;
	}

	bool editor_widget_inspector_t::is_component_paste_enabled(sid_t type_id) const
	{
		return _copied_component_stream.get_size() != 0 && _copied_component_type == type_id;
	}

	void editor_widget_inspector_t::copy_entity_info()
	{
		_copied_entity_info		  = {};
		_copied_entity_info_valid = false;

		if (_edit_world.is_null() || _display_entities.empty())
			return;

		world_t& world			  = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		_copied_entity_info		  = editor_commands_entity_info_t::read(world, _display_entities.front());
		_copied_entity_info_valid = true;
	}

	void editor_widget_inspector_t::copy_component(sid_t type_id)
	{
		_copied_component_stream.destroy();
		_copied_component_type = 0;

		if (_edit_world.is_null() || _display_entities.empty())
			return;

		world_t&			   world = editor_world_controller_t::get().get_editor_world(_edit_world)->get_world();
		ecs_component_table_t* table = world.find_component_table(type_id);
		if (table == nullptr || !ecs_t::table_has(*table, _display_entities.front()))
			return;

		const void* component = ecs_t::table_get(*table, _display_entities.front());
		if (reflection_registry_t::get().find_type(type_id) == nullptr)
			return;

		if (!reflection_registry_t::get().type_to_stream(type_id, const_cast<void*>(component), nullptr, _copied_component_stream))
			return;
		_copied_component_type = type_id;
	}

}
