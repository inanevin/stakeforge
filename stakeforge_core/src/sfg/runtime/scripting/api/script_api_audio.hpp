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

#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	u8	 api_audio_play(world_t* world, entity_id_t entity);
	u8	 api_audio_pause(world_t* world, entity_id_t entity);
	u8	 api_audio_stop(world_t* world, entity_id_t entity);
	void api_audio_pause_all(world_t* world);
	void api_audio_resume_all(world_t* world);

	struct script_api_audio_t
	{
		u32 size										= 0;
		u32 version										= 0;
		u8 (*play)(world_t* world, entity_id_t entity)	= nullptr;
		u8 (*pause)(world_t* world, entity_id_t entity) = nullptr;
		u8 (*stop)(world_t* world, entity_id_t entity)	= nullptr;
		void (*pause_all)(world_t* world)				= nullptr;
		void (*resume_all)(world_t* world)				= nullptr;
	};

	const script_api_audio_t& get_script_api_audio();
}
