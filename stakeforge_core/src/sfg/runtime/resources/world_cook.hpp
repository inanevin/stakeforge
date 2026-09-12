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

#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;
	class world_t;

	class world_cooker_t
	{
	public:
		static void		   world_to_stream(const world_t& world, ostream_t& out_stream);
		static void		   world_to_json(const world_t& world, nlohmann::json& out_json);
		static void		   world_from_json(world_t& world, const nlohmann::json& in_json);
		static entity_id_t entity_from_stream(world_t& world, istream_t& in_stream, bool generate_new_guids);
		static entity_id_t entity_from_json(world_t& world, const nlohmann::json& in_json);

		static entity_id_t spawn_prefab(world_t& world, resource_handle_t prefab_handle, const vector_t<entity_guid_t>& reserved_guids);
		static void		   make_prefab_chain(world_t& world, entity_id_t entity, resource_handle_t prefab_handle);
		static void		   break_prefab_chain(world_t& world, entity_id_t entity);

		static void entity_to_stream(const world_t& world, entity_id_t entity, ostream_t& out_stream);
		static void entity_into_world_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json);
		static void entity_to_prefab_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json);
	};
}
