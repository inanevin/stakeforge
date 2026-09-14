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

#include "animation_library.hpp"
#include "animation_library_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/delfrrr/delaunator.hpp>

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
			.layer_count = static_cast<u32>(def.layer_count),
		};

		if (runtime.layer_count == 0)
		{
			SFG_ERR("animation library has no layers: {0}", entry.hash);
			return false;
		}

		for (u32 layer_index = 0; layer_index < runtime.layer_count; ++layer_index)
		{
			const animation_library_layer_def_t& source_layer = def.layers[layer_index];
			const u32							 state_count  = static_cast<u32>(source_layer.states.size());
			const chunk_handle32_t				 layer_handle{
				.head = entry.runtime.head + static_cast<u32>(offsetof(animation_library_runtime_t, layers)) + static_cast<u32>(sizeof(animation_library_layer_runtime_t)) * layer_index,
				.size = sizeof(animation_library_layer_runtime_t),
			};

			animation_library_layer_runtime_t& layer = runtime.layers[layer_index];

			layer = {
				.name_hash			  = TO_SID(static_cast<const char*>(source_layer.name)),
				.mask				  = source_layer.mask,
				.state_count		  = state_count,
				.default_active_state = source_layer.default_active_state,
				.weight				  = source_layer.weight,
			};

			if (state_count == 0)
				continue;

			layer.states = memory.allocate_bytes(sizeof(animation_library_state_runtime_t) * state_count, alignof(animation_library_state_runtime_t));

			animation_library_state_runtime_t* states = memory.get<animation_library_state_runtime_t>(layer.states);

			for (u32 state_index = 0; state_index < state_count; ++state_index)
			{
				const animation_library_state_def_t& source_state = source_layer.states[state_index];
				const u32							 clip_count	  = static_cast<u32>(source_state.clip_count);
				const chunk_handle32_t				 state_handle{
					.head = layer.states.head + static_cast<u32>(sizeof(animation_library_state_runtime_t)) * state_index,
					.size = sizeof(animation_library_state_runtime_t),
				};

				animation_library_state_runtime_t& target_state = states[state_index];

				target_state = {
					.name_hash			 = TO_SID(static_cast<const char*>(source_state.name)),
					.initial_blend_value = source_state.blend_value,
					.layer				 = layer_handle,
					.clip_count			 = clip_count,
					.speed				 = source_state.speed,
					.blend_type			 = source_state.blend_type,
					.loop				 = source_state.loop,
				};

				for (u32 clip = 0; clip < clip_count; ++clip)
				{
					const animation_library_clip_def_t& source_clip = source_state.clips[clip];

					target_state.clips[clip] = {
						.animation_clip = source_clip.animation_clip,
						.weight_value	= source_clip.blend_position,
						.state			= state_handle,
						.start_time		= source_clip.start_time,
						.playback_speed = source_clip.playback_speed,
					};
				}

				if (target_state.blend_type == animation_library_blend_type_e::blend_1d)
					std::sort(target_state.clips, target_state.clips + target_state.clip_count, [](const animation_library_clip_runtime_t& a, const animation_library_clip_runtime_t& b) -> bool { return a.weight_value.x < b.weight_value.x; });
				else if (target_state.blend_type == animation_library_blend_type_e::blend_2d && clip_count > 2)
				{
					// delaunay triangulation for barycentric
					vector_t<double> triangle_points = {};

					for (u32 clip = 0; clip < clip_count; ++clip)
					{
						const animation_library_clip_def_t& clip_def = source_state.clips[clip];
						triangle_points.push_back(clip_def.blend_position.x);
						triangle_points.push_back(clip_def.blend_position.y);
					}

					delaunator::Delaunator						 d(triangle_points);
					animation_library_state_delaunay_triangle_t* tris = nullptr;

					if (!d.triangles.empty())
					{
						target_state.delaunay_triangles = memory.allocate<animation_library_state_delaunay_triangle_t>(d.triangles.size() / 3);
						tris							= memory.get<animation_library_state_delaunay_triangle_t>(target_state.delaunay_triangles);
						target_state.triangle_count		= static_cast<u32>(d.triangles.size() / 3);
					}

					auto find_clip_idx = [&](const vec2f_t& p) -> u32 {
						u32 idx = 0;

						for (u32 clip = 0; clip < clip_count; ++clip)
						{
							const animation_library_clip_def_t& clip_def = source_state.clips[clip];

							if (p.equals(clip_def.blend_position))
								return idx;
							idx++;
						}

						return UINT32_MAX;
					};

					for (size_t i = 0; i < d.triangles.size(); i += 3)
					{
						const float x0 = d.coords[2 * d.triangles[i]];
						const float y0 = d.coords[2 * d.triangles[i] + 1];
						const float x1 = d.coords[2 * d.triangles[i + 1]];
						const float y1 = d.coords[2 * d.triangles[i + 1] + 1];
						const float x2 = d.coords[2 * d.triangles[i + 2]];
						const float y2 = d.coords[2 * d.triangles[i + 2] + 1];

						const vec2f_t								 v1	 = {x1, y1};
						const vec2f_t								 v2	 = {x2, y2};
						animation_library_state_delaunay_triangle_t& tri = tris[i / 3];
						tri.v0											 = {x0, y0};

						tri.clip_index0 = find_clip_idx(tri.v0);
						tri.clip_index1 = find_clip_idx(v1);
						tri.clip_index2 = find_clip_idx(v2);

						const vec2f_t edge0	  = v1 - tri.v0;
						const vec2f_t edge1	  = v2 - tri.v0;
						const f32	  inv_det = 1.0f / (edge0.x * edge1.y - edge0.y * edge1.x);
						tri.coeff1			  = {edge1.y * inv_det, -edge1.x * inv_det};
						tri.coeff2			  = {-edge0.y * inv_det, edge0.x * inv_det};
					}
				}
			}
		}

		return true;
	}

	void animation_library_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&			 memory	 = ctx.resource_manager.get_memory();
		animation_library_runtime_t& runtime = *memory.get<animation_library_runtime_t>(entry.runtime);

		for (u32 layer_index = 0; layer_index < runtime.layer_count; ++layer_index)
		{
			const animation_library_layer_runtime_t& layer = runtime.layers[layer_index];

			if (layer.state_count != 0)
			{
				const animation_library_state_runtime_t* st = memory.get<animation_library_state_runtime_t>(layer.states);
				for (u32 i = 0; i < layer.state_count; i++)
				{
					if (st[i].delaunay_triangles)
						memory.free(st[i].delaunay_triangles);
				}
				memory.free(layer.states);
			}
		}

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
