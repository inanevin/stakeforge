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
		j["guid"]		 = header.guid;
		j["parent_guid"] = header.parent_guid;
		j["name"]		 = header.name;
		j["local_pos"]	 = nlohmann::json::array_t({header.local_pos.x, header.local_pos.y, header.local_pos.z});
		j["local_rot"]	 = nlohmann::json::array_t({header.local_rot.x, header.local_rot.y, header.local_rot.z, header.local_rot.w});
		j["local_scale"] = nlohmann::json::array_t({header.local_scale.x, header.local_scale.y, header.local_scale.z});
		j["prefab"]		 = header.prefab;
	}

	void from_json(const nlohmann::json& j, world_cook_entity_header_t& header)
	{
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
