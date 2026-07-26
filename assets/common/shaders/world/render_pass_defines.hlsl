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
