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

#include "world_cook_entity_header.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void world_cook_entity_header_t::serialize(ostream_t& stream) const
	{
		stream << guid << parent_guid << name << local_pos << local_rot << local_scale << prefab;
	}

	void world_cook_entity_header_t::deserialize(istream_t& stream)
	{
		stream >> guid >> parent_guid >> name >> local_pos >> local_rot >> local_scale >> prefab;
	}

	void to_json(nlohmann::json& j, const world_cook_entity_header_t& header)
	{
		nlohmann::json prefab_entities = nlohmann::json::array();
		for (entity_guid_t g : header.prefab_entity_guids)
			prefab_entities.push_back(g);

		j["prefab_entity_guids"] = prefab_entities;
		j["guid"]				 = header.guid;
		j["parent_guid"]		 = header.parent_guid;
		j["name"]				 = header.name;
		j["local_pos"]			 = nlohmann::json::array_t({header.local_pos.x, header.local_pos.y, header.local_pos.z});
		j["local_rot"]			 = nlohmann::json::array_t({header.local_rot.x, header.local_rot.y, header.local_rot.z, header.local_rot.w});
		j["local_scale"]		 = nlohmann::json::array_t({header.local_scale.x, header.local_scale.y, header.local_scale.z});
		j["prefab"]				 = header.prefab;
	}

	void from_json(const nlohmann::json& j, world_cook_entity_header_t& header)
	{
		header.prefab_entity_guids.resize(0);

		if (const auto it = j.find("prefab_entity_guids"); it != j.end() && it->is_array())
		{
			header.prefab_entity_guids.reserve(it->size());

			for (const nlohmann::json& guid_json : *it)
				header.prefab_entity_guids.push_back(guid_json.get<entity_guid_t>());
		}

		header.guid						 = j.value<entity_guid_t>("guid", NULL_ENTITY_GUID);
		header.parent_guid				 = j.value<entity_guid_t>("parent_guid", NULL_ENTITY_GUID);
		header.name						 = j.value<string_t>("name", "");
		header.prefab					 = j.value<resource_handle_t>("prefab", NULL_RESOURCE_HANDLE);
		const nlohmann::json local_pos	 = j.value("local_pos", nlohmann::json::array());
		const nlohmann::json local_rot	 = j.value("local_rot", nlohmann::json::array());
		const nlohmann::json local_scale = j.value("local_scale", nlohmann::json::array());
		header.local_pos				 = local_pos.is_array() && local_pos.size() >= 3 ? vec3f_t{local_pos.at(0).get<f32>(), local_pos.at(1).get<f32>(), local_pos.at(2).get<f32>()} : vec3f_t::zero;
		header.local_rot				 = local_rot.is_array() && local_rot.size() >= 4 ? quat_t{local_rot.at(0).get<f32>(), local_rot.at(1).get<f32>(), local_rot.at(2).get<f32>(), local_rot.at(3).get<f32>()} : quat_t{};
		header.local_scale				 = local_scale.is_array() && local_scale.size() >= 3 ? vec3f_t{local_scale.at(0).get<f32>(), local_scale.at(1).get<f32>(), local_scale.at(2).get<f32>()} : vec3f_t::one;
	}
}
