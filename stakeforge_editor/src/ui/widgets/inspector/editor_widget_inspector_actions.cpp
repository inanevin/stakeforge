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
#include "ui/widgets/inspector/editor_widget_inspector.hpp"
#include "world/editor_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_component_add.hpp"
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

	void editor_widget_inspector_t::open_add_component_modal()
	{
		if (_allow_prefab_blocks && is_selection_prefab_child())
			return;

		const span_t<const entity_id_t> entities = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().get_selected_entities();

		if (entities.size != 0)
			_component_modal->open(*_ui, _edit_world, entities);
	}

	void editor_widget_inspector_t::create_add_component_button()
	{
		const editor_theme_t& theme			 = editor_theme_t::get();
		_add_component_button				 = new editor_widget_button_t();
		editor_widget_button_config_t config = {};
		config.text							 = "Add Component";
		config.width						 = {.mode = editor_widget_width_e::fixed, .value = theme.item_width * 1.5f};
		_add_component_button->init(*_ui, _column, config);

		ui::layout_in_t& button_in = _ui->get_tree().in(_add_component_button->get_root());
		button_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		button_in.pos_value.x	   = 0.5f;
		button_in.anchor_x		   = ui::anchor_e::center;

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

		if (table == nullptr || !table->has(_display_entities.front()))
			return;

		const void* component = table->get(_display_entities.front());

		if (reflection_registry_t::get().find_type(type_id) == nullptr)
			return;

		if (!reflection_registry_t::get().type_to_stream(type_id, const_cast<void*>(component), nullptr, _copied_component_stream))
			return;
		_copied_component_type = type_id;
	}

}
