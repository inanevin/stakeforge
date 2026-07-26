// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//     1. Redistributions of source code must retain the above copyright notice, this
//        list of conditions and the following disclaimer.
//  
//     2. Redistributions in binary form must reproduce the above copyright notice,
//        this list of conditions and the following disclaimer in the documentation
//        and/or other materials provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
//  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"
#include "depth.hlsl"
#include "light.hlsl"
#include "clustered_lighting.hlsl"

float3 get_cluster_view_ray(float2 uv, render_pass_data_view view_data)
{
	const float3 world_far = reconstruct_world_position(uv, 0.0, view_data.inv_view_proj);
	const float3 view_far = mul(view_data.view, float4(world_far, 1.0)).xyz;

	return view_far / abs(view_far.z);
}

void get_cluster_aabb(uint3 cluster_id, render_pass_data_view view_data, out float3 aabb_min, out float3 aabb_max)
{
	// build this cluster's view-space bounds
	const float2 pixel_min = float2(cluster_id.xy * view_data.cluster_dims.w);
	const float2 pixel_max = min(pixel_min + view_data.cluster_dims.w, view_data.viewport_size.xy);
	const float2 uv_min = pixel_min / view_data.viewport_size.xy;
	const float2 uv_max = pixel_max / view_data.viewport_size.xy;
	const float slice_near = view_data.cluster_depth.x * pow(view_data.cluster_depth.y / view_data.cluster_depth.x, cluster_id.z / (float)view_data.cluster_dims.z);
	const float slice_far = view_data.cluster_depth.x * pow(view_data.cluster_depth.y / view_data.cluster_depth.x, (cluster_id.z + 1.0) / view_data.cluster_dims.z);
	const float3 rays[4] = {
		get_cluster_view_ray(float2(uv_min.x, uv_min.y), view_data),
		get_cluster_view_ray(float2(uv_max.x, uv_min.y), view_data),
		get_cluster_view_ray(float2(uv_min.x, uv_max.y), view_data),
		get_cluster_view_ray(float2(uv_max.x, uv_max.y), view_data),
	};

	aabb_min = 3.402823466e+38.xxx;
	aabb_max = -3.402823466e+38.xxx;

	[unroll]
	for (uint i = 0; i < 4; ++i)
	{
		const float3 near_point = rays[i] * slice_near;
		const float3 far_point = rays[i] * slice_far;

		aabb_min = min(aabb_min, min(near_point, far_point));
		aabb_max = max(aabb_max, max(near_point, far_point));
	}
}

bool sphere_intersects_aabb(float3 center, float radius, float3 aabb_min, float3 aabb_max)
{
	const float3 closest = clamp(center, aabb_min, aabb_max);
	const float3 delta = center - closest;

	return dot(delta, delta) <= radius * radius;
}

[numthreads(4, 4, 1)]
void CSMain(uint3 dispatch_id : SV_DispatchThreadID)
{
	// fetch the view and shared lighting resources
	const render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
	const render_pass_data_lighting lighting_data = sfg_get_cbv<render_pass_data_lighting>(SFG_RENDER_PASS_LIGHTING);

	if (any(dispatch_id >= view_data.cluster_dims.xyz))
		return;

	StructuredBuffer<gpu_light> light_buffer = sfg_get_ssbo<gpu_light>(lighting_data.light_buffer_index);
	RWStructuredBuffer<gpu_light_cluster> cluster_buffer = sfg_get_rws_buffer<gpu_light_cluster>(lighting_data.cluster_buffer_uav_index);
	RWStructuredBuffer<uint> cluster_light_indices = sfg_get_rws_buffer<uint>(lighting_data.cluster_light_indices_buffer_uav_index);

	float3 cluster_min = 0.0.xxx;
	float3 cluster_max = 0.0.xxx;
	get_cluster_aabb(dispatch_id, view_data, cluster_min, cluster_max);

	const uint local_cluster_index = dispatch_id.x + view_data.cluster_dims.x * (dispatch_id.y + view_data.cluster_dims.y * dispatch_id.z);
	const uint cluster_index = view_data.cluster_buffer_offset + local_cluster_index;
	const uint light_offset = view_data.cluster_light_indices_buffer_offset + local_cluster_index * view_data.cluster_light_capacity;
	const uint local_light_offset = lighting_data.light_counts.x;
	const uint local_light_count = lighting_data.light_counts.y + lighting_data.light_counts.z + lighting_data.light_counts.w;
	const uint spot_light_end = lighting_data.light_counts.x + lighting_data.light_counts.y + lighting_data.light_counts.z;
	uint matching_light_count = 0;

	// pack every local light touching this cluster
	[loop]
	for (uint i = 0; i < local_light_count; ++i)
	{
		const uint light_index = local_light_offset + i;
		const gpu_light light = light_buffer[light_index];
		const float3 center = mul(view_data.view, float4(light.position_range.xyz, 1.0)).xyz;
		float radius = light.position_range.w;

		if (light_index >= spot_light_end && radius > 0.0)
			radius += length(float2(abs(light.direction_param0.w), abs(light.right_param1.w)));

		if (radius > 0.0 && !sphere_intersects_aabb(center, radius, cluster_min, cluster_max))
			continue;

		if (matching_light_count < view_data.cluster_light_capacity)
			cluster_light_indices[light_offset + matching_light_count] = light_index;

		++matching_light_count;
	}

	// publish this cluster's compact light range
	gpu_light_cluster cluster;
	cluster.light_offset = light_offset;
	cluster.light_count = min(matching_light_count, view_data.cluster_light_capacity);
	cluster.overflow = matching_light_count > view_data.cluster_light_capacity ? 1 : 0;
	cluster.pad = 0;
	cluster_buffer[cluster_index] = cluster;
}
