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
#include "commands/editor_command_project_settings.hpp"
#include "editor_command_system.hpp"
#include "editor_project.hpp"
#include "editor_app.hpp"
#include <sfg/io/log.hpp>

namespace sfg
{
	namespace
	{
		bool apply_and_save_project_settings(const editor_project_settings_data_t& settings)
		{
			editor_command_project_settings_t::apply(settings);
			return editor_project_t::get().save(editor_project_t::get()._runtime.path.c_str());
		}

		bool project_settings_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_edit_project_settings_payload_t& payload = system.get_payload_as<editor_command_edit_project_settings_payload_t>(command);
			return apply_and_save_project_settings(payload.previous);
		}

		bool project_settings_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_edit_project_settings_payload_t& payload = system.get_payload_as<editor_command_edit_project_settings_payload_t>(command);
			return apply_and_save_project_settings(payload.post);
		}
	}

	editor_project_settings_data_t editor_command_project_settings_t::read()
	{
		const editor_project_t& project = editor_project_t::get();
		return {
			.runtime_settings = project.runtime_settings,
			.last_world_guid  = project.last_world_guid,
		};
	}

	void editor_command_project_settings_t::apply(const editor_project_settings_data_t& settings)
	{
		editor_project_t& project = editor_project_t::get();
		project.runtime_settings  = settings.runtime_settings;
		project.last_world_guid	  = settings.last_world_guid;
		editor_app_t::get().get_runtime().update_settings(settings.runtime_settings);
	}

	bool editor_command_project_settings_t::edit(const editor_project_settings_data_t& previous, const editor_project_settings_data_t& post)
	{
		if (previous == post)
			return true;

		const editor_command_edit_project_settings_payload_t payload{
			.previous = previous,
			.post	  = post,
		};

		const editor_command_issue_desc_t desc{
			.undo		= project_settings_undo,
			.redo		= project_settings_redo,
			.debug_name = "Edit Project Settings",
			.type		= editor_command_type_e::project_settings_edit,
		};

		const editor_command_handle_t handle = editor_command_system_t::get().issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue project settings edit command");
			return false;
		}

		return true;
	}
}
