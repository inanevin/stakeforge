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

#include "editor_command_animation_library.hpp"

#include "editor_command_system.hpp"
#include "ui/panels/editor_panel_animation_library.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	namespace
	{
		struct animation_library_edit_payload_t
		{
			chunk_handle32_t previous_stream = {};
			chunk_handle32_t post_stream	 = {};
		};

		bool apply_library_stream(editor_command_system_t& system, editor_panel_animation_library_t& panel, chunk_handle32_t handle)
		{
			animation_library_def_t definition = {};
			istream_t				stream(system.get_aux_data().get<u8>(handle), handle.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_library_def_t>::value, &definition, nullptr, stream))
			{
				SFG_ERR("failed to deserialize animation library edit");
				return false;
			}

			panel.apply_edits(definition);

			return true;
		}

		bool animation_library_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const animation_library_edit_payload_t& payload = system.get_payload_as<animation_library_edit_payload_t>(command);
			editor_panel_animation_library_t&		panel	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

			return apply_library_stream(system, panel, payload.previous_stream);
		}

		bool animation_library_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const animation_library_edit_payload_t& payload = system.get_payload_as<animation_library_edit_payload_t>(command);
			editor_panel_animation_library_t&		panel	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

			return apply_library_stream(system, panel, payload.post_stream);
		}

		bool animation_library_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			animation_library_edit_payload_t& payload = system.get_payload_as<animation_library_edit_payload_t>(command);

			system.get_aux_data().free(payload.previous_stream);
			system.get_aux_data().free(payload.post_stream);
			payload = {};

			return true;
		}
	}

	bool editor_command_animation_library_edit_t::submit(editor_command_system_t& system, editor_panel_animation_library_t& panel, const animation_library_def_t& definition, const char* debug_name)
	{
		ostream_t			   previous_stream = {};
		ostream_t			   post_stream	   = {};
		reflection_registry_t& registry		   = reflection_registry_t::get();

		if (!registry.type_to_stream(type_id_t<animation_library_def_t>::value, const_cast<animation_library_def_t*>(&panel.get_library_def()), nullptr, previous_stream) ||
			!registry.type_to_stream(type_id_t<animation_library_def_t>::value, const_cast<animation_library_def_t*>(&definition), nullptr, post_stream))
		{
			SFG_ERR("failed to serialize animation library edit");
			return false;
		}

		if (previous_stream.get_size() == post_stream.get_size() && SFG_MEMCMP(previous_stream.get_raw(), post_stream.get_raw(), post_stream.get_size()) == 0)
			return true;

		const animation_library_edit_payload_t payload{
			.previous_stream = system.get_aux_data().allocate_bytes(previous_stream.get_size(), alignof(u8)),
			.post_stream	 = system.get_aux_data().allocate_bytes(post_stream.get_size(), alignof(u8)),
		};

		SFG_MEMCPY(system.get_aux_data().get<u8>(payload.previous_stream), previous_stream.get_raw(), previous_stream.get_size());
		SFG_MEMCPY(system.get_aux_data().get<u8>(payload.post_stream), post_stream.get_raw(), post_stream.get_size());

		const editor_command_issue_desc_t desc{
			.undo		= animation_library_edit_undo,
			.redo		= animation_library_edit_redo,
			.cleanup	= animation_library_edit_cleanup,
			.user_data	= &panel,
			.debug_name = debug_name,
			.type		= editor_command_type_e::animation_library_edit,
			.run_redo	= false,
		};
		const editor_command_handle_t handle = system.issue_command(desc, payload);

		if (handle.is_null())
		{
			system.get_aux_data().free(payload.previous_stream);
			system.get_aux_data().free(payload.post_stream);
			SFG_ERR("failed to issue animation library edit command");
			return false;
		}

		panel.apply_edits(definition);

		return true;
	}
}
