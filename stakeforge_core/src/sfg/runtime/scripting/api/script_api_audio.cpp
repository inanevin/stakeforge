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

#include "script_api_audio.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	u8 api_audio_play(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		const ecs_component_table_t& source_table = world->get_component_table(type_id_t<component_audio_source_t>::value);

		if (!source_table.has(entity))
			return 0;

		return world->get_audio_controller().play(entity) ? 1 : 0;
	}

	u8 api_audio_pause(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		const ecs_component_table_t& system_source_table = world->get_component_table(type_id_t<component_system_audio_source_t>::value);

		if (!system_source_table.has(entity))
			return 0;

		world->get_audio_controller().pause(entity);
		return 1;
	}

	u8 api_audio_stop(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		const ecs_component_table_t& system_source_table = world->get_component_table(type_id_t<component_system_audio_source_t>::value);

		if (!system_source_table.has(entity))
			return 0;

		world->get_audio_controller().stop(entity);
		return 1;
	}

	void api_audio_pause_all(world_t* world)
	{
		SFG_ASSERT(world != nullptr);

		world->get_audio_controller().pause_all();
	}

	void api_audio_resume_all(world_t* world)
	{
		SFG_ASSERT(world != nullptr);

		world->get_audio_controller().resume_all();
	}

	const script_api_audio_t& get_script_api_audio()
	{
		static const script_api_audio_t api{
			.size		= static_cast<u32>(sizeof(script_api_audio_t)),
			.version	= 1,
			.play		= api_audio_play,
			.pause		= api_audio_pause,
			.stop		= api_audio_stop,
			.pause_all	= api_audio_pause_all,
			.resume_all = api_audio_resume_all,
		};

		return api;
	}
}
