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

#include "animation_graph_cook.hpp"
#include "animation_graph_def.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool animation_graph_cooker::cook_from_def(const animation_graph_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t def_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_graph_def_t>::value, const_cast<animation_graph_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize animation graph definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::animation_graph,
			.magic		 = animation_graph_loader_t::WIRE_MAGIC,
			.version	 = animation_graph_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		vector_t<resource_dependency_t> dependencies = {};

		if (def.target_skeleton != NULL_RESOURCE_HANDLE)
		{
			dependencies.push_back({
				.handle = def.target_skeleton,
				.type	= resource_type_e::skeleton,
			});
		}

		for (const animation_graph_node_def_t& node : def.nodes)
		{
			if (node.type != animation_graph_node_type_e::asm_node)
				continue;

			for (const u32 bone_index : node.asm_node.masked_bones)
			{
				if (bone_index >= MAX_SKELETON_BONES)
				{
					SFG_ERR("animation graph masked bone index is out of range: {0}", bone_index);
					return false;
				}
			}

			for (const animation_graph_asm_state_def_t& state : node.asm_node.states)
			{
				if (state.state_type == animation_graph_asm_state_type_e::blend_2d)
				{
					if (state.clips.size() > UINT16_MAX)
					{
						SFG_ERR("2D animation blend state has too many clips: {0}", state.name);
						return false;
					}

					for (u32 clip_index = 0; clip_index < state.clips.size(); ++clip_index)
					{
						for (u32 other_clip_index = clip_index + 1; other_clip_index < state.clips.size(); ++other_clip_index)
						{
							if (state.clips[clip_index].blend_value_2d.equals(state.clips[other_clip_index].blend_value_2d))
							{
								SFG_ERR("2D animation blend state has duplicate clip positions: {0}", state.name);
								return false;
							}
						}
					}
				}

				for (const animation_graph_clip_def_t& clip : state.clips)
				{
					if (clip.playback_speed <= 0.0f)
					{
						SFG_ERR("animation graph clip playback speed must be greater than zero");
						return false;
					}

					if (clip.clip == NULL_RESOURCE_HANDLE)
						continue;

					const auto dependency_it = std::find_if(dependencies.begin(), dependencies.end(), [&](const resource_dependency_t& dependency) { return dependency.handle == clip.clip && dependency.type == resource_type_e::animation; });

					if (dependency_it != dependencies.end())
						continue;

					dependencies.push_back({
						.handle = clip.clip,
						.type	= resource_type_e::animation,
					});
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
