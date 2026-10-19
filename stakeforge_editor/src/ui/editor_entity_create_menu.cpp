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

#include "ui/editor_entity_create_menu.hpp"
#include "ui/editor_action_menu_common.hpp"
#include "commands/editor_commands_entity.hpp"
#include "commands/editor_command_primitive_spawn.hpp"
#include "commands/editor_commands_world_edit_context.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"

namespace sfg
{
	namespace
	{
		enum class create_command_e : u16
		{
			empty = 2000,
			camera,
			directional_light,
			point_light,
			spot_light,
			area_light,
			ground,
			character,
			cube,
			sphere,
			cylinder,
			capsule,
			plane,
			folder,
		};
	}

	span_t<const editor_action_menu_row_desc_t> editor_entity_create_menu_t::get_rows(bool allow_folder)
	{
		static const editor_action_menu_row_desc_t primitives[] = {
			{
				.text	 = "Cube",
				.command = static_cast<u16>(create_command_e::cube),
			},
			{
				.text	 = "Plane",
				.command = static_cast<u16>(create_command_e::plane),
			},
			{
				.text	 = "Sphere",
				.command = static_cast<u16>(create_command_e::sphere),
			},
			{
				.text	 = "Cylinder",
				.command = static_cast<u16>(create_command_e::cylinder),
			},
			{
				.text	 = "Capsule",
				.command = static_cast<u16>(create_command_e::capsule),
			},
		};
		static const editor_action_menu_row_desc_t lights[] = {
			{
				.text	 = "Directional",
				.command = static_cast<u16>(create_command_e::directional_light),
			},
			{
				.text	 = "Point",
				.command = static_cast<u16>(create_command_e::point_light),
			},
			{
				.text	 = "Spot",
				.command = static_cast<u16>(create_command_e::spot_light),
			},
			{
				.text	 = "Area",
				.command = static_cast<u16>(create_command_e::area_light),
			},
		};
		static const editor_action_menu_row_desc_t rows[] = {
			{
				.text	 = "Empty Entity",
				.command = static_cast<u16>(create_command_e::empty),
			},
			{
				.text		 = "Primitives",
				.children	 = primitives,
				.child_count = 5,
			},
			{
				.text	 = "Camera",
				.command = static_cast<u16>(create_command_e::camera),
			},
			{
				.text		 = "Lights",
				.children	 = lights,
				.child_count = 4,
			},
			{
				.text	 = "Ground",
				.command = static_cast<u16>(create_command_e::ground),
			},
			{
				.text	 = "Character",
				.command = static_cast<u16>(create_command_e::character),
			},
			{
				.text	 = "Folder",
				.command = static_cast<u16>(create_command_e::folder),
			},
		};

		return {.data = rows, .size = allow_folder ? 7u : 6u};
	}

	bool editor_entity_create_menu_t::execute(u16 command, editor_world_handle_t handle, entity_id_t parent, editor_world_folder_handle_t folder)
	{
		if (command < static_cast<u16>(create_command_e::empty) || command > static_cast<u16>(create_command_e::folder))
			return false;

		editor_world_controller_t& controller = editor_world_controller_t::get();

		if (handle.is_null())
			return true;

		editor_world_t&				 editor_world = *controller.get_editor_world(handle);
		editor_world_edit_context_t& context	  = editor_world.get_edit_context();

		if (context.get_play_mode() != editor_play_mode_e::none || context.get_edit_type() != editor_world_edit_type_e::full_control)
			return true;

		if (parent != NULL_ENTITY_ID && !context.is_entity_child_insertion_allowed(editor_world.get_world(), parent))
			return true;

		if (command == static_cast<u16>(create_command_e::folder))
		{
			if (parent == NULL_ENTITY_ID)
				editor_commands_world_edit_context_t::create_folder(handle, "Folder", folder);
		}
		else if (command >= static_cast<u16>(create_command_e::cube))
		{
			const editor_primitive_type_e primitive = static_cast<editor_primitive_type_e>(command - static_cast<u16>(create_command_e::cube));
			editor_command_primitive_spawn_t::spawn(handle, primitive, parent, folder);
		}
		else
		{
			const editor_entity_preset_e preset = static_cast<editor_entity_preset_e>(command - static_cast<u16>(create_command_e::empty));
			editor_commands_entity_t::create(handle, parent, folder, preset);
		}

		if (parent != NULL_ENTITY_ID)
			context.set_entity_folded(editor_world.get_world().get_entity_guid(parent), false);

		if (!folder.is_null())
			context.set_folder_folded(folder, false);

		return true;
	}
}
