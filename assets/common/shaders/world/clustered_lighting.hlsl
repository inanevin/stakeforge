// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
//  
//  Author: Inan Evin
//  http://www.inanevin.com
//  
//  Copyright (c) [2025-] [Inan Evin]
//  
//  Stakeforge is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  Stakeforge is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.
//
//  As an additional permission under section 7 of GPLv3, the copyright
//  holders grant the Stakeforge Game Linking Exception, version 1.0,
//  in GAME-LINKING-EXCEPTION.md.
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
