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

#include "commands/editor_commands_audio.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		bool apply_audio_config(const editor_command_audio_edit_payload_t& payload, const audio_cook_config_t& config)
		{
			nlohmann::json cook_options = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<audio_cook_config_t>::value, const_cast<audio_cook_config_t*>(&config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize audio cook options for asset {0}", payload.audio_id);
				return false;
			}

			return editor_asset_manager_t::get().save_and_cook_file_asset_options_async(payload.audio_id, cook_options);
		}

		bool audio_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_audio_edit_payload_t& payload = system.get_payload_as<editor_command_audio_edit_payload_t>(command);
			return apply_audio_config(payload, payload.previous);
		}

		bool audio_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_audio_edit_payload_t& payload = system.get_payload_as<editor_command_audio_edit_payload_t>(command);
			return apply_audio_config(payload, payload.post);
		}
	}

	bool editor_command_audio_edit_t::edit(sid_t audio_id, const audio_cook_config_t& previous, const audio_cook_config_t& post)
	{
		if (previous == post)
			return true;

		const editor_command_audio_edit_payload_t payload{
			.previous = previous,
			.post	  = post,
			.audio_id = audio_id,
		};
		const editor_command_issue_desc_t desc{
			.undo		= audio_edit_undo,
			.redo		= audio_edit_redo,
			.debug_name = "Audio Edit",
			.type		= editor_command_type_e::audio_edit,
			.notify		= false,
		};
		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue audio edit command");
			return false;
		}

		return true;
	}
}
