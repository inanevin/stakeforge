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

float3 get_cluster_view_ray(float2 uv, render_pass_data_lighting rp_data)
{
	const float3 world_far = reconstruct_world_position(uv, 0.0, rp_data.inv_view_proj);
	const float3 view_far = mul(rp_data.view, float4(world_far, 1.0)).xyz;

	return view_far / abs(view_far.z);
}

void get_cluster_aabb(uint3 cluster_id, render_pass_data_lighting rp_data, out float3 aabb_min, out float3 aabb_max)
{
	const float2 pixel_min = float2(cluster_id.xy * rp_data.cluster_dims.w);
	const float2 pixel_max = min(pixel_min + rp_data.cluster_dims.w, rp_data.cluster_screen.xy);
	const float2 uv_min = pixel_min / rp_data.cluster_screen.xy;
	const float2 uv_max = pixel_max / rp_data.cluster_screen.xy;
	const float slice_near = rp_data.cluster_depth.x * pow(rp_data.cluster_depth.y / rp_data.cluster_depth.x, cluster_id.z / (float)rp_data.cluster_dims.z);
	const float slice_far = rp_data.cluster_depth.x * pow(rp_data.cluster_depth.y / rp_data.cluster_depth.x, (cluster_id.z + 1.0) / rp_data.cluster_dims.z);
	const float3 rays[4] = {
		get_cluster_view_ray(float2(uv_min.x, uv_min.y), rp_data),
		get_cluster_view_ray(float2(uv_max.x, uv_min.y), rp_data),
		get_cluster_view_ray(float2(uv_min.x, uv_max.y), rp_data),
		get_cluster_view_ray(float2(uv_max.x, uv_max.y), rp_data),
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
	const render_pass_data_lighting rp_data = sfg_get_cbv<render_pass_data_lighting>(sfg_constant_rp0);

	if (any(dispatch_id >= rp_data.cluster_dims.xyz))
		return;

	StructuredBuffer<gpu_light> light_buffer = sfg_get_ssbo<gpu_light>(sfg_constant_rp1);
	RWStructuredBuffer<gpu_light_cluster> cluster_buffer = sfg_get_rws_buffer<gpu_light_cluster>(sfg_constant_rp2);
	RWStructuredBuffer<uint> cluster_light_indices = sfg_get_rws_buffer<uint>(sfg_constant_rp3);

	float3 cluster_min = 0.0.xxx;
	float3 cluster_max = 0.0.xxx;
	get_cluster_aabb(dispatch_id, rp_data, cluster_min, cluster_max);

	const uint cluster_index = dispatch_id.x + rp_data.cluster_dims.x * (dispatch_id.y + rp_data.cluster_dims.y * dispatch_id.z);
	const uint light_offset = cluster_index * rp_data.cluster_screen.w;
	const uint local_light_offset = rp_data.light_counts.x;
	const uint local_light_count = rp_data.light_counts.y + rp_data.light_counts.z + rp_data.light_counts.w;
	const uint spot_light_end = rp_data.light_counts.x + rp_data.light_counts.y + rp_data.light_counts.z;
	uint matching_light_count = 0;

	// pack every local light touching this cluster
	[loop]
	for (uint i = 0; i < local_light_count; ++i)
	{
		const uint light_index = local_light_offset + i;
		const gpu_light light = light_buffer[light_index];
		const float3 center = mul(rp_data.view, float4(light.position_range.xyz, 1.0)).xyz;
		float radius = light.position_range.w;

		if (light_index >= spot_light_end && radius > 0.0)
			radius += length(float2(abs(light.direction_param0.w), abs(light.right_param1.w)));

		if (radius > 0.0 && !sphere_intersects_aabb(center, radius, cluster_min, cluster_max))
			continue;

		if (matching_light_count < rp_data.cluster_screen.w)
			cluster_light_indices[light_offset + matching_light_count] = light_index;

		++matching_light_count;
	}

	gpu_light_cluster cluster;
	cluster.light_offset = light_offset;
	cluster.light_count = min(matching_light_count, rp_data.cluster_screen.w);
	cluster.overflow = matching_light_count > rp_data.cluster_screen.w ? 1 : 0;
	cluster.pad = 0;
	cluster_buffer[cluster_index] = cluster;
}
