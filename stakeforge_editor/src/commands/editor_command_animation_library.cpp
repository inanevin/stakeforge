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
#include "ui/panels/animation_library/editor_panel_animation_library.hpp"

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
			chunk_handle32_t previous_stream									   = {};
			chunk_handle32_t post_stream										   = {};
			u32				 previous_layer										   = UINT32_MAX;
			u32				 post_layer											   = UINT32_MAX;
			u32				 previous_state										   = UINT32_MAX;
			u32				 post_state											   = UINT32_MAX;
			u32				 previous_clip										   = UINT32_MAX;
			u32				 post_clip											   = UINT32_MAX;
			bool			 previous_layer_expanded[MAX_ANIMATION_LIBRARY_LAYERS] = {};
			bool			 post_layer_expanded[MAX_ANIMATION_LIBRARY_LAYERS]	   = {};
		};

		bool apply_library_stream(editor_command_system_t& system, editor_panel_animation_library_t& panel, chunk_handle32_t handle, u32 selected_layer, const bool* layer_expanded, u32 selected_state, u32 selected_clip)
		{
			animation_library_def_t definition = {};
			istream_t				stream(system.get_aux_data().get<u8>(handle), handle.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_library_def_t>::value, &definition, nullptr, stream))
			{
				SFG_ERR("failed to deserialize animation library edit");
				return false;
			}

			panel.apply_edits(definition, selected_layer, layer_expanded, selected_state, selected_clip);

			return true;
		}

		bool animation_library_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const animation_library_edit_payload_t& payload = system.get_payload_as<animation_library_edit_payload_t>(command);
			editor_panel_animation_library_t&		panel	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

			return apply_library_stream(system, panel, payload.previous_stream, payload.previous_layer, payload.previous_layer_expanded, payload.previous_state, payload.previous_clip);
		}

		bool animation_library_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const animation_library_edit_payload_t& payload = system.get_payload_as<animation_library_edit_payload_t>(command);
			editor_panel_animation_library_t&		panel	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

			return apply_library_stream(system, panel, payload.post_stream, payload.post_layer, payload.post_layer_expanded, payload.post_state, payload.post_clip);
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

	bool editor_command_animation_library_edit_t::begin(editor_panel_animation_library_t& panel)
	{
		if (panel._edit_previous_stream.size != 0)
			return true;

		ostream_t stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_library_def_t>::value, &panel._library, nullptr, stream))
		{
			SFG_ERR("failed to serialize animation library edit");
			return false;
		}

		chunk_allocator_t& aux = editor_command_system_t::get().get_aux_data();

		panel._edit_previous_stream = aux.allocate_bytes(stream.get_size(), alignof(u8));
		panel._edit_previous_layer	= panel._selected_layer;
		panel._edit_previous_state	= panel._selected_state;
		panel._edit_previous_clip	= panel._selected_clip;
		SFG_MEMCPY(panel._edit_previous_layer_expanded, panel._layer_expanded, sizeof(panel._layer_expanded));
		SFG_MEMCPY(aux.get<u8>(panel._edit_previous_stream), stream.get_raw(), stream.get_size());

		return true;
	}

	bool editor_command_animation_library_edit_t::submit(editor_panel_animation_library_t& panel, const char* debug_name)
	{
		SFG_ASSERT(panel._edit_previous_stream.size != 0);

		editor_command_system_t& system		 = editor_command_system_t::get();
		chunk_allocator_t&		 aux		 = system.get_aux_data();
		ostream_t				 post_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_library_def_t>::value, &panel._library, nullptr, post_stream))
		{
			SFG_ERR("failed to serialize animation library edit");
			apply_library_stream(system, panel, panel._edit_previous_stream, panel._edit_previous_layer, panel._edit_previous_layer_expanded, panel._edit_previous_state, panel._edit_previous_clip);
			aux.free(panel._edit_previous_stream);
			panel._edit_previous_stream = {};
			return false;
		}

		if (panel._edit_previous_layer == panel._selected_layer && panel._edit_previous_state == panel._selected_state && panel._edit_previous_clip == panel._selected_clip && panel._edit_previous_stream.size == post_stream.get_size() &&
			SFG_MEMCMP(aux.get<u8>(panel._edit_previous_stream), post_stream.get_raw(), post_stream.get_size()) == 0)
		{
			aux.free(panel._edit_previous_stream);
			panel._edit_previous_stream = {};
			return true;
		}

		animation_library_edit_payload_t payload{
			.previous_stream = panel._edit_previous_stream,
			.post_stream	 = aux.allocate_bytes(post_stream.get_size(), alignof(u8)),
			.previous_layer	 = panel._edit_previous_layer,
			.post_layer		 = panel._selected_layer,
			.previous_state	 = panel._edit_previous_state,
			.post_state		 = panel._selected_state,
			.previous_clip	 = panel._edit_previous_clip,
			.post_clip		 = panel._selected_clip,
		};

		SFG_MEMCPY(aux.get<u8>(payload.post_stream), post_stream.get_raw(), post_stream.get_size());
		SFG_MEMCPY(payload.previous_layer_expanded, panel._edit_previous_layer_expanded, sizeof(payload.previous_layer_expanded));
		SFG_MEMCPY(payload.post_layer_expanded, panel._layer_expanded, sizeof(payload.post_layer_expanded));

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
			apply_library_stream(system, panel, payload.previous_stream, payload.previous_layer, payload.previous_layer_expanded, payload.previous_state, payload.previous_clip);
			aux.free(payload.previous_stream);
			aux.free(payload.post_stream);
			panel._edit_previous_stream = {};
			SFG_ERR("failed to issue animation library edit command");
			return false;
		}

		panel._edit_previous_stream = {};

		return true;
	}

	bool editor_command_animation_library_edit_t::select(editor_panel_animation_library_t& panel, u32 selected_layer, u32 selected_state, u32 selected_clip)
	{
		if (panel._selected_layer == selected_layer && panel._selected_state == selected_state && panel._selected_clip == selected_clip)
			return true;

		struct selection_payload_t
		{
			u32 previous_layer = UINT32_MAX;
			u32 post_layer	   = UINT32_MAX;
			u32 previous_state = UINT32_MAX;
			u32 post_state	   = UINT32_MAX;
			u32 previous_clip  = UINT32_MAX;
			u32 post_clip	   = UINT32_MAX;
		};

		const selection_payload_t payload{
			.previous_layer = panel._selected_layer,
			.post_layer		= selected_layer,
			.previous_state = panel._selected_state,
			.post_state		= selected_state,
			.previous_clip	= panel._selected_clip,
			.post_clip		= selected_clip,
		};
		const editor_command_issue_desc_t desc{
			.undo =
				[](editor_command_system_t& system, editor_command_t& command) {
					const selection_payload_t&		  selection = system.get_payload_as<selection_payload_t>(command);
					editor_panel_animation_library_t& target	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

					target.apply_selection(selection.previous_layer, selection.previous_state, selection.previous_clip);

					return true;
				},
			.redo =
				[](editor_command_system_t& system, editor_command_t& command) {
					const selection_payload_t&		  selection = system.get_payload_as<selection_payload_t>(command);
					editor_panel_animation_library_t& target	= *static_cast<editor_panel_animation_library_t*>(command.user_data);

					target.apply_selection(selection.post_layer, selection.post_state, selection.post_clip);

					return true;
				},
			.user_data	= &panel,
			.debug_name = "Select Animation Library Item",
			.type		= editor_command_type_e::animation_library_select_layer,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue animation library selection command");
			return false;
		}

		return true;
	}
}
