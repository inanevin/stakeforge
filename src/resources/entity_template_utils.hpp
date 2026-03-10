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

#pragma once

#include "common/string_id.hpp"
#include "data/vector.hpp"
#include "data/static_vector.hpp"
#include "data/string.hpp"
#include "data/hash_map.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "resources/common_resources.hpp"
#include "world/world_constants.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json_fwd.hpp>
#endif

namespace SFG
{
	class world;
	class entity_manager;
	class component_manager;
	class resource_manager;
	struct entity_template_entity_raw;

	struct entity_template_utils
	{
#ifdef SFG_TOOLMODE
		static void append_entity_components_as_json(
			nlohmann::json& out_components_array, world_handle entity, u32 entity_index, entity_manager& em, component_manager& cm, resource_manager& rm, const hash_map<u32, i32>& index_by_world, vector<string>& out_resource_paths);
		static void						  component_json_to_component_buffer(const nlohmann::json& comp_json, ostream& out_buffer);
		static void						  entity_components_to_component_buffer(world_handle entity, u32 entity_index, entity_manager& em, component_manager& cm, resource_manager& rm, const hash_map<u32, i32>& index_by_world, ostream& out_buffer);
		static entity_template_entity_raw entity_to_entity_template_entity_raw(world_handle entity, entity_manager& em, resource_manager& rm, const hash_map<u32, i32>& index_by_world);
#endif

		static void fill_components_from_buffer(istream& in, const static_vector<world_handle, 1024>& created, component_manager& cm, resource_manager& rm, world& w);
	};
}
