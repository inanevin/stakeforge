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

#include <sfg/memory/chunk_handle.hpp>

namespace sfg
{
	class editor_panel_ragdoll_viewer_t;

	struct editor_command_ragdoll_edit_payload_t
	{
		chunk_handle32_t previous_stream = {};
		chunk_handle32_t post_stream	 = {};
	};

	class editor_command_ragdoll_edit_t final
	{
	public:
		editor_command_ragdoll_edit_t()												   = delete;
		~editor_command_ragdoll_edit_t()											   = delete;
		editor_command_ragdoll_edit_t(const editor_command_ragdoll_edit_t&)			   = delete;
		editor_command_ragdoll_edit_t& operator=(const editor_command_ragdoll_edit_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static bool begin(editor_panel_ragdoll_viewer_t& viewer);
		static bool submit(editor_panel_ragdoll_viewer_t& viewer, const char* debug_name, bool notify);
		static void cancel(editor_panel_ragdoll_viewer_t& viewer);
	};
}
