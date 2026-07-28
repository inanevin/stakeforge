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
