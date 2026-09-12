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

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct world_cook_entity_header_t
	{
		vector_t<entity_guid_t> prefab_entity_guids;
		entity_guid_t			guid		= NULL_ENTITY_GUID;
		entity_guid_t			parent_guid = NULL_ENTITY_GUID;
		string_t				name		= {};
		vec3f_t					local_pos	= vec3f_t::zero;
		quat_t					local_rot	= {};
		vec3f_t					local_scale = vec3f_t::one;
		resource_handle_t		prefab		= NULL_RESOURCE_HANDLE;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	void to_json(nlohmann::json& j, const world_cook_entity_header_t& header);
	void from_json(const nlohmann::json& j, world_cook_entity_header_t& header);
}
