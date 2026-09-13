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

#include "animation_library_cook.hpp"
#include "animation_library.hpp"
#include "animation_library_def.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool animation_library_cooker::cook_from_def(const animation_library_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t def_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_library_def_t>::value, const_cast<animation_library_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize animation library definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::animation_library,
			.magic		 = animation_library_loader_t::WIRE_MAGIC,
			.version	 = animation_library_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		size_t dependency_capacity = 1;

		for (size_t layer_index = 0; layer_index < def.layer_count; ++layer_index)
		{
			const animation_library_layer_def_t& layer = def.layers[layer_index];

			for (const animation_library_state_def_t& state : layer.states)
				dependency_capacity += state.clip_count;
		}

		vector_t<resource_dependency_t> dependencies = {};

		dependencies.reserve(dependency_capacity);

		if (def.skeleton != NULL_RESOURCE_HANDLE)
			dependencies.push_back({.handle = def.skeleton, .type = resource_type_e::skeleton});

		for (size_t layer_index = 0; layer_index < def.layer_count; ++layer_index)
		{
			const animation_library_layer_def_t& layer = def.layers[layer_index];

			for (const animation_library_state_def_t& state : layer.states)
			{
				for (u32 clip_index = 0; clip_index < state.clip_count; ++clip_index)
				{
					const resource_handle_t clip = state.clips[clip_index].animation_clip;

					if (clip == NULL_RESOURCE_HANDLE)
						continue;

					const auto it = std::find_if(dependencies.begin(), dependencies.end(), [clip](const resource_dependency_t& dependency) { return dependency.handle == clip && dependency.type == resource_type_e::animation; });

					if (it == dependencies.end())
						dependencies.push_back({.handle = clip, .type = resource_type_e::animation});
				}
			}
		}

		out_header.dependency_count = static_cast<u32>(dependencies.size());

		for (const resource_dependency_t& dependency : dependencies)
			stream << dependency;

		stream.write_raw(def_stream.get_raw(), def_stream.get_size());

		return true;
	}
}
