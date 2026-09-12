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

#pragma once

#include "editor_project.hpp"
#include "editor_settings.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/chunk_allocator.hpp>

namespace sfg
{
	struct editor_command_project_settings_data_t
	{
		editor_project_settings_data_t project = {};
		editor_settings_configurable_t editor  = {};

		bool operator==(const editor_command_project_settings_data_t&) const = default;
	};

	struct editor_command_edit_project_settings_payload_t
	{
		chunk_handle32_t previous_project_settings = {};
		chunk_handle32_t post_project_settings	   = {};
		chunk_handle32_t previous_editor_settings  = {};
		chunk_handle32_t post_editor_settings	   = {};
		sid_t			 previous_last_world_guid  = NULL_SID;
		sid_t			 post_last_world_guid	   = NULL_SID;
	};

	class editor_command_project_settings_t final
	{
	public:
		editor_command_project_settings_t() = delete;

		static editor_command_project_settings_data_t read();
		static void									  apply(const editor_command_project_settings_data_t& settings);
		static bool									  edit(const editor_command_project_settings_data_t& previous, const editor_command_project_settings_data_t& post);
	};
}
