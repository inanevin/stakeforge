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

#include "sprite_common.hlsl"
#include "light.hlsl"
#include "clustered_lighting.hlsl"

SFG_MATERIAL_PARAM_VEC4("tint", sfg_color)
SFG_MATERIAL_PARAM_F32("alpha_cutoff", 0.5, 0.0, 1.0)

SamplerComparisonState smp_shadow : static_sampler_shadow_2d;
SamplerComparisonState smp_shadow_cube : static_sampler_shadow_cube;

#ifdef USE_SELECTION

float4 PSMain(vs_output IN) : SV_TARGET
{
    sample_sprite(IN);
    return float4(1.0, 1.0, 1.0, 1.0);
}

#elif defined(WRITE_ID)

uint PSMain(vs_output IN) : SV_TARGET
{
    sample_sprite(IN);
    return IN.entity_id;
}

#elif defined(USE_ZPREPASS)

void PSMain(vs_output IN)
{
    sample_sprite(IN);
}

#elif defined(USE_GBUFFER)

struct ps_output
{
    float4 rt0 : SV_Target0;
    float4 rt1 : SV_Target1;
    float4 rt2 : SV_Target2;
    float4 rt3 : SV_Target3;
};

ps_output PSMain(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    float4 color = sample_sprite(IN);

    if (color.a < mat_data.alpha_cutoff)
        discard;

    ps_output OUT;
    OUT.rt0 = float4(color.rgb, 1.0);
    OUT.rt1 = float4(oct_encode(IN.world_normal), 0.0, 0.0);
    OUT.rt2 = float4(1.0, 1.0, 0.0, 0.0);
    OUT.rt3 = float4(0.0, 0.0, 0.0, 1.0);
    return OUT;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    render_pass_data_lighting lighting_data = sfg_get_cbv<render_pass_data_lighting>(SFG_RENDER_PASS_LIGHTING);
    render_pass_data_fog fog_data = sfg_get_cbv<render_pass_data_fog>(SFG_RENDER_PASS_FOG);
    StructuredBuffer<gpu_light> light_buffer = sfg_get_ssbo<gpu_light>(lighting_data.light_buffer_index);
    StructuredBuffer<gpu_shadow_view> shadow_buffer = sfg_get_ssbo<gpu_shadow_view>(lighting_data.shadow_buffer_index);
    StructuredBuffer<gpu_reflection_probe> reflection_probe_buffer = sfg_get_ssbo<gpu_reflection_probe>(lighting_data.reflection_probe_buffer_index);
    StructuredBuffer<gpu_light_cluster> cluster_buffer = sfg_get_ssbo<gpu_light_cluster>(lighting_data.cluster_buffer_index);
    StructuredBuffer<uint> cluster_light_indices = sfg_get_ssbo<uint>(lighting_data.cluster_light_indices_buffer_index);
    float4 color = sample_sprite(IN);
    float view_depth = abs(mul(view_data.view, float4(IN.world_pos, 1.0)).z);
    uint cluster_index = view_data.cluster_buffer_offset + get_light_cluster_index((uint2)IN.pos.xy, view_depth, view_data.cluster_dims, view_data.cluster_depth);
    gpu_light_cluster cluster = cluster_buffer[cluster_index];

    if (lighting_data.debug_cluster_heatmap != 0)
        return float4(get_light_cluster_heatmap(cluster.light_count, cluster.overflow), color.a);

    surface_lighting_data surface;
    surface.world_pos = IN.world_pos;
    surface.view_direction = normalize(view_data.camera_pos.xyz - IN.world_pos);
    surface.normal = IN.world_normal;
    surface.albedo = color.rgb;
    surface.ambient_occlusion = 1.0;
    surface.roughness = 1.0;
    surface.metallic = 0.0;

    scene_lighting_data scene;
    scene.view = view_data.view;
    scene.light_counts = lighting_data.light_counts;

    float3 final_color = lighting_data.ambient_color.rgb * color.rgb;
    final_color += evaluate_clustered_scene_lighting(
        surface,
        scene,
        light_buffer,
        shadow_buffer,
        cluster_light_indices,
        cluster.light_offset,
        cluster.light_count,
        smp_shadow,
        smp_shadow_cube);

    if ((view_data.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_REFLECTIONS) != 0)
    {
        final_color += evaluate_reflection_probe_lighting(
            surface,
            view_data.camera_pos.xyz,
            reflection_probe_buffer,
            lighting_data.reflection_probe_count,
            lighting_data.brdf_lut_index,
            smp_linear);
    }

    if ((view_data.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG) != 0)
        final_color = apply_fog(final_color, IN.world_pos, view_data.camera_pos.xyz, fog_data);

    return float4(final_color, color.a);
}

#endif
