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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	class world_render_context_t;
	struct world_render_prep_data_t;
	struct world_render_reflection_allocation_t;
	struct world_render_snapshot_t;

	struct world_render_clustered_lighting_data_t
	{
		gfx_handle_t command_buffer				  = {};
		gfx_handle_t cluster_buffer				  = {};
		gfx_handle_t cluster_light_indices_buffer = {};
		gfx_handle_t shader						  = {};
		gpu_index_t	 lighting_data_index		  = NULL_GPU_INDEX;
		gpu_index_t	 global_cbv_index			  = NULL_GPU_INDEX;
	};

	struct world_render_clustered_lighting_view_t
	{
		gpu_index_t view_data_index = NULL_GPU_INDEX;
		u32			cluster_count_x = 0;
		u32			cluster_count_y = 0;
		u32			cluster_count_z = 0;
	};

	class world_rendering_t final
	{
	public:
		static void render_world(world_render_context_t& ctx, const world_render_snapshot_t& snapshot, world_render_prep_data_t& prep_data, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);

	private:
		static void render_depth_prepass(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_gbuffer(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_shadows(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_probe(gfx_handle_t								 command_buffer,
								 const world_render_context_t&				 ctx,
								 const world_render_snapshot_t&				 snapshot,
								 const world_render_prep_data_t&			 prep_data,
								 const world_render_reflection_allocation_t& allocation,
								 const u16*									 cull_view_indices,
								 bool										 skybox_only,
								 u8											 frame_index,
								 gpu_index_t								 global_cbv_index,
								 gfx_handle_t								 global_layout);
		static void render_clustered_lighting(const world_render_clustered_lighting_data_t& data, span_t<const world_render_clustered_lighting_view_t> views);
		static void render_prefilter_diffuse_sh(const world_render_context_t& ctx, const world_render_reflection_allocation_t& allocation, u8 frame_index, gpu_index_t global_cbv_index);
		static void render_ssao(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index);
		static void render_lighting(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_forward(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		static void render_bloom(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index);
		static void render_post_process(const world_render_context_t& ctx, const world_render_snapshot_t& snapshot, const world_render_prep_data_t& prep_data, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
	};
}
