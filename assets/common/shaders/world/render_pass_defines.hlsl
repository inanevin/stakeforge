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

#define SFG_RENDER_PASS_VIEW sfg_constant_rp0
#define SFG_RENDER_PASS_ENTITIES sfg_constant_rp1
#define SFG_RENDER_PASS_BONES sfg_constant_rp2
#define SFG_RENDER_PASS_LIGHTING sfg_constant_rp3
#define SFG_RENDER_PASS_SPECIFIC sfg_constant_rp4
#define SFG_RENDER_PASS_FOG sfg_constant_rp5

static const uint SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_REFLECTIONS = 1u << 0;
static const uint SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG = 1u << 1;

struct render_pass_data_view
{
    float4x4 view;
    float4x4 view_proj;
    float4x4 inv_view;
    float4x4 inv_view_proj;
    float4 camera_pos;
    float4 cluster_depth;
    uint4 cluster_dims;
    float2 viewport_size;
    float2 inv_viewport_size;
    float near_plane;
    float far_plane;
    uint depth_texture_index;
    uint cluster_buffer_offset;
    uint cluster_light_indices_buffer_offset;
    uint cluster_light_capacity;
    uint flags;
    uint pad;
};

struct render_pass_data_lighting
{
    float4 ambient_color;
    uint4 light_counts;
    uint light_buffer_index;
    uint shadow_buffer_index;
    uint reflection_probe_buffer_index;
    uint cluster_buffer_index;
    uint cluster_buffer_uav_index;
    uint cluster_light_indices_buffer_index;
    uint cluster_light_indices_buffer_uav_index;
    uint reflection_probe_count;
    float environment_intensity;
    uint brdf_lut_index;
    uint debug_cluster_heatmap;
    uint pad;
};

struct render_pass_data_fog
{
    float4 color_intensity;
    float4 distance_height;
    float height_falloff;
    float max_opacity;
    uint type;
    uint pad;
};

struct render_pass_data_deferred_lighting
{
    uint gbuffer_albedo_index;
    uint gbuffer_normal_index;
    uint gbuffer_orm_index;
    uint gbuffer_emissive_index;
    uint ambient_occlusion_index;
    uint3 pad;
};
