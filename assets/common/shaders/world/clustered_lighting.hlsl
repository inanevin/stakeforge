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

struct gpu_light_cluster
{
	uint light_offset;
	uint light_count;
	uint overflow;
	uint pad;
};

uint get_light_cluster_depth_slice(float view_depth, float4 cluster_depth, uint depth_slice_count)
{
	const float slice = log2(max(view_depth, cluster_depth.x)) * cluster_depth.z + cluster_depth.w;

	return min((uint)max(slice, 0.0), depth_slice_count - 1);
}

uint get_light_cluster_index(uint2 pixel, float view_depth, uint4 cluster_dims, float4 cluster_depth)
{
	const uint tile_x = min(pixel.x / cluster_dims.w, cluster_dims.x - 1);
	const uint tile_y = min(pixel.y / cluster_dims.w, cluster_dims.y - 1);
	const uint tile_z = get_light_cluster_depth_slice(view_depth, cluster_depth, cluster_dims.z);

	return tile_x + cluster_dims.x * (tile_y + cluster_dims.y * tile_z);
}

float3 get_light_cluster_heatmap(uint light_count, uint overflow)
{
	if (overflow != 0)
		return float3(1.0, 0.0, 1.0);

	const float count = (float)light_count;
	float3 color = lerp(float3(0.02, 0.03, 0.12), float3(0.0, 0.5, 1.0), saturate(count / 4.0));
	color = lerp(color, float3(0.0, 1.0, 0.2), saturate((count - 4.0) / 4.0));
	color = lerp(color, float3(1.0, 0.9, 0.0), saturate((count - 8.0) / 8.0));
	color = lerp(color, float3(1.0, 0.05, 0.0), saturate((count - 16.0) / 16.0));

	return color;
}
