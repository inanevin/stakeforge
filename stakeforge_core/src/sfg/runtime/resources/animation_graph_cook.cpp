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
				for (const animation_graph_clip_def_t& clip : state.clips)
				{
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
