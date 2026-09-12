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

#include "animation_library.hpp"
#include "animation_library_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool animation_library_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read animation library resource: {0}", entry.hash);
			return false;
		}

		animation_library_def_t def = {};
		istream_t				stream(file_stream.get_raw(), file_stream.get_size());

		if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_library_def_t>::value, &def, nullptr, stream))
		{
			SFG_ERR("failed to deserialize animation library definition: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t&			 memory	 = ctx.resource_manager.get_memory();
		animation_library_runtime_t& runtime = *memory.get<animation_library_runtime_t>(entry.runtime);

		runtime = {
			.skeleton	 = def.skeleton,
			.layer_count = static_cast<u32>(def.layers.size()),
		};

		if (runtime.layer_count == 0)
		{
			SFG_ERR("animation library has no layers: {0}", entry.hash);
			return false;
		}

		for (const animation_library_layer_def_t& layer : def.layers)
		{
			runtime.state_count += static_cast<u32>(layer.states.size());

			for (const animation_library_state_def_t& state : layer.states)
				runtime.clip_count += static_cast<u32>(state.clip_count);
		}

		runtime.layers = memory.allocate_bytes(sizeof(animation_library_layer_runtime_t) * runtime.layer_count, alignof(animation_library_layer_runtime_t));

		if (runtime.state_count != 0)
			runtime.states = memory.allocate_bytes(sizeof(animation_library_state_runtime_t) * runtime.state_count, alignof(animation_library_state_runtime_t));

		if (runtime.clip_count != 0)
			runtime.clips = memory.allocate_bytes(sizeof(animation_library_clip_runtime_t) * runtime.clip_count, alignof(animation_library_clip_runtime_t));

		animation_library_layer_runtime_t* layers	   = memory.get<animation_library_layer_runtime_t>(runtime.layers);
		animation_library_state_runtime_t* states	   = runtime.state_count != 0 ? memory.get<animation_library_state_runtime_t>(runtime.states) : nullptr;
		animation_library_clip_runtime_t*  clips	   = runtime.clip_count != 0 ? memory.get<animation_library_clip_runtime_t>(runtime.clips) : nullptr;
		u32								   state_index = 0;
		u32								   clip_index  = 0;

		for (u32 layer_index = 0; layer_index < runtime.layer_count; ++layer_index)
		{
			const animation_library_layer_def_t& source_layer = def.layers[layer_index];
			const u32							 state_count  = static_cast<u32>(source_layer.states.size());
			const chunk_handle32_t				 layer_handle{
				.head = runtime.layers.head + static_cast<u32>(sizeof(animation_library_layer_runtime_t)) * layer_index,
				.size = sizeof(animation_library_layer_runtime_t),
			};

			layers[layer_index] = {
				.name_hash		= TO_SID(static_cast<const char*>(source_layer.name)),
				.mask_name_hash = source_layer.use_mask ? TO_SID(static_cast<const char*>(source_layer.mask_name)) : NULL_SID,
				.states =
					{
						.head = runtime.states.head + static_cast<u32>(sizeof(animation_library_state_runtime_t)) * state_index,
						.size = static_cast<u32>(sizeof(animation_library_state_runtime_t)) * state_count,
					},
				.state_count		  = state_count,
				.default_active_state = source_layer.default_active_state,
				.weight				  = source_layer.weight,
				.use_mask			  = source_layer.use_mask,
			};

			for (const animation_library_state_def_t& source_state : source_layer.states)
			{
				const u32			   clip_count = static_cast<u32>(source_state.clip_count);
				const chunk_handle32_t state_handle{
					.head = runtime.states.head + static_cast<u32>(sizeof(animation_library_state_runtime_t)) * state_index,
					.size = sizeof(animation_library_state_runtime_t),
				};

				states[state_index] = {
					.name_hash	 = TO_SID(static_cast<const char*>(source_state.name)),
					.blend_value = source_state.blend_value,
					.layer		 = layer_handle,
					.clips =
						{
							.head = runtime.clips.head + static_cast<u32>(sizeof(animation_library_clip_runtime_t)) * clip_index,
							.size = static_cast<u32>(sizeof(animation_library_clip_runtime_t)) * clip_count,
						},
					.clip_count = clip_count,
					.blend_type = source_state.blend_type,
				};

				for (u32 source_clip_index = 0; source_clip_index < clip_count; ++source_clip_index)
				{
					const animation_library_clip_def_t& source_clip = source_state.clips[source_clip_index];

					clips[clip_index] = {
						.animation_clip = source_clip.animation_clip,
						.weight_value	= source_clip.weight_value,
						.state			= state_handle,
					};

					++clip_index;
				}

				++state_index;
			}
		}

		return true;
	}

	void animation_library_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&			 memory	 = ctx.resource_manager.get_memory();
		animation_library_runtime_t& runtime = *memory.get<animation_library_runtime_t>(entry.runtime);

		if (runtime.clip_count != 0)
			memory.free(runtime.clips);

		if (runtime.state_count != 0)
			memory.free(runtime.states);

		memory.free(runtime.layers);

		runtime = {};
	}

	const resource_type_desc_t animation_library_resource_desc = {
		.type				 = resource_type_e::animation_library,
		.runtime_size		 = sizeof(animation_library_runtime_t),
		.runtime_alignment	 = alignof(animation_library_runtime_t),
		.internals_size		 = sizeof(animation_library_internals_t),
		.internals_alignment = alignof(animation_library_internals_t),
		.wire_magic			 = animation_library_loader_t::WIRE_MAGIC,
		.wire_version		 = animation_library_loader_t::WIRE_VERSION,
		.load				 = animation_library_loader_t::load,
		.unload				 = animation_library_loader_t::unload,
	};
}
