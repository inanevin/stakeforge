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

#include <sfg/runtime/resources/audio_cook.hpp>

namespace sfg
{
	struct editor_command_audio_edit_payload_t
	{
		audio_cook_config_t previous = {};
		audio_cook_config_t post	 = {};
		sid_t				audio_id = NULL_SID;
	};

	class editor_command_audio_edit_t final
	{
	public:
		editor_command_audio_edit_t() = delete;

		static bool edit(sid_t audio_id, const audio_cook_config_t& previous, const audio_cook_config_t& post);
	};
}
