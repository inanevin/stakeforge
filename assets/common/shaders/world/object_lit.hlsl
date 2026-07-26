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
#include "packing_utils.hlsl"
#include "entity.hlsl"
#include "normal.hlsl"
#include "bone.hlsl"
#include "render_pass_defines.hlsl"
#include "light.hlsl"
#include "clustered_lighting.hlsl"

SFG_MATERIAL_TEXTURE("albedo", sfg_texture2d)
SFG_MATERIAL_TEXTURE("normal", sfg_texture2d)
SFG_MATERIAL_TEXTURE("orm", sfg_texture2d)
SFG_MATERIAL_TEXTURE("emissive", sfg_texture2d)
SFG_MATERIAL_SAMPLER("albedo")
SFG_MATERIAL_SAMPLER("normal")
SFG_MATERIAL_SAMPLER("orm")
SFG_MATERIAL_SAMPLER("emissive")
SFG_MATERIAL_PARAM_VEC4("base_color_factor", sfg_color)
SFG_MATERIAL_PARAM_VEC4("emissive_factor", sfg_color)
SFG_MATERIAL_PARAM_F32("ao_multiplier", 1.0, 0.0, 1.0)
SFG_MATERIAL_PARAM_F32("roughness_multiplier", 1.0, 0.0, 1.0)
SFG_MATERIAL_PARAM_F32("metallic_multiplier", 1.0, 0.0, 1.0)
SFG_MATERIAL_PARAM_F32("normal_strength", 1.0, 0.0, 2.0)
SFG_MATERIAL_PARAM_F32("emissive_multiplier", 1.0, 0.0, 1000.0)
SFG_MATERIAL_PARAM_F32("alpha_cutoff", 0.5, 0.0, 1.0)
SFG_MATERIAL_PARAM_VEC4("albedo_tiling_offset", sfg_pack_uint2)
SFG_MATERIAL_PARAM_VEC4("normal_tiling_offset", sfg_pack_uint2)
SFG_MATERIAL_PARAM_VEC4("orm_tiling_offset", sfg_pack_uint2)
SFG_MATERIAL_PARAM_VEC4("emissive_tiling_offset", sfg_pack_uint2)

SamplerComparisonState smp_shadow : static_sampler_shadow_2d;
SamplerComparisonState smp_shadow_cube : static_sampler_shadow_cube;

#ifdef USE_SKINNING

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
    float4 bone_weights : BLENDWEIGHT0;
    uint4 bone_indices : BLENDINDICES0;
};

#else

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

#endif

#ifdef USE_ZPREPASS

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

#else

struct vs_output
{
    float4 pos : SV_POSITION;
    float3 world_pos : TEXCOORD0;
    float3 world_norm : TEXCOORD1;
    float3 world_tan : TEXCOORD2;
    float3 world_bit : TEXCOORD3;
    float2 uv : TEXCOORD4;
};

#endif

vs_output VSMain(vs_input IN)
{
    vs_output OUT;
    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(SFG_RENDER_PASS_ENTITIES);

    gpu_entity entity = entity_buffer[sfg_constant_obj0];

    float4 obj_pos;
#ifndef USE_ZPREPASS
    float3 obj_norm;
    float3 obj_tan;
#endif

#ifdef USE_SKINNING

    StructuredBuffer<gpu_bone> bone_buffer = sfg_get_ssbo<gpu_bone>(SFG_RENDER_PASS_BONES);

    float4 skinned_pos = float4(0, 0, 0, 0);
#ifndef USE_ZPREPASS
    float3 skinned_normal = float3(0, 0, 0);
    float3 skinned_tangent = float3(0, 0, 0);
#endif

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index = sfg_constant_obj1 + IN.bone_indices[i];
        float weight = IN.bone_weights[i];
        float4x4 bone_mat = bone_buffer[bone_index].bone;

        skinned_pos += mul(bone_mat, float4(IN.pos, 1.0)) * weight;
#ifndef USE_ZPREPASS
        skinned_normal += mul((float3x3)bone_mat, IN.normal) * weight;
        skinned_tangent += mul((float3x3)bone_mat, IN.tangent.xyz) * weight;
#endif
    }

    obj_pos = skinned_pos;
#ifndef USE_ZPREPASS
    obj_norm = skinned_normal;
    obj_tan = skinned_tangent;
#endif
#else
    obj_pos = float4(IN.pos, 1.0);
#ifndef USE_ZPREPASS
    obj_norm = IN.normal;
    obj_tan = IN.tangent.xyz;
#endif
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(view_data.view_proj, float4(world_pos, 1.0));
    OUT.uv = IN.uv;

#ifndef USE_ZPREPASS
    float3 world_normal = normalize(mul(entity.normal_matrix, float4(obj_norm, 0.0)).xyz);
    float3 world_tangent = normalize(mul(entity.normal_matrix, float4(obj_tan, 0.0)).xyz);
    world_tangent = normalize(world_tangent - world_normal * dot(world_normal, world_tangent));

    OUT.world_pos = world_pos;
    OUT.world_norm = world_normal;
    OUT.world_tan = world_tangent;
    OUT.world_bit = normalize(cross(world_normal, world_tangent)) * IN.tangent.w;
#endif

    return OUT;
}

struct material_data
{
    float4 base_color_factor;
    float4 emissive_factor;
    float ao_multiplier;
    float roughness_multiplier;
    float metallic_multiplier;
    float normal_strength;
    float emissive_multiplier;
    float alpha_cutoff;
    uint2 albedo_tiling_offset;
    uint2 normal_tiling_offset;
    uint2 orm_tiling_offset;
    uint2 emissive_tiling_offset;
};

float4 sample_albedo(vs_output IN, material_data mat_data)
{
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState sampler_albedo = sfg_get_sampler_state(sfg_constant_mat5);
    float2 tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);

    return tex_albedo.Sample(sampler_albedo, IN.uv * tiling + offset) * mat_data.base_color_factor;
}

#ifndef USE_ZPREPASS

struct material_surface
{
    float4 albedo;
    float3 normal;
    float3 emissive;
    float ambient_occlusion;
    float roughness;
    float metallic;
};

material_surface sample_material_surface(vs_output IN, material_data mat_data)
{
    Texture2D tex_normal = sfg_get_texture<Texture2D>(sfg_constant_mat2);
    Texture2D tex_orm = sfg_get_texture<Texture2D>(sfg_constant_mat3);
    Texture2D tex_emissive = sfg_get_texture<Texture2D>(sfg_constant_mat4);
    SamplerState sampler_normal = sfg_get_sampler_state(sfg_constant_mat6);
    SamplerState sampler_orm = sfg_get_sampler_state(sfg_constant_mat7);
    SamplerState sampler_emissive = sfg_get_sampler_state(sfg_constant_mat8);

    float2 normal_tiling = unpack_half2x16(mat_data.normal_tiling_offset.x);
    float2 normal_offset = unpack_half2x16(mat_data.normal_tiling_offset.y);
    float2 orm_tiling = unpack_half2x16(mat_data.orm_tiling_offset.x);
    float2 orm_offset = unpack_half2x16(mat_data.orm_tiling_offset.y);
    float2 emissive_tiling = unpack_half2x16(mat_data.emissive_tiling_offset.x);
    float2 emissive_offset = unpack_half2x16(mat_data.emissive_tiling_offset.y);

    float3 tangent_normal = tex_normal.Sample(sampler_normal, IN.uv * normal_tiling + normal_offset).xyz * 2.0 - 1.0;
    float2 normal_xy = tangent_normal.xy * mat_data.normal_strength;
    tangent_normal = float3(normal_xy, sqrt(saturate(1.0 - dot(normal_xy, normal_xy))));

    float3x3 tbn = float3x3(IN.world_tan, IN.world_bit, IN.world_norm);
    float3 orm = tex_orm.Sample(sampler_orm, IN.uv * orm_tiling + orm_offset).rgb;

    material_surface surface;
    surface.albedo = sample_albedo(IN, mat_data);
    surface.normal = normalize(mul(tangent_normal, tbn));
    surface.emissive = tex_emissive.Sample(sampler_emissive, IN.uv * emissive_tiling + emissive_offset).rgb * mat_data.emissive_factor.rgb * mat_data.emissive_multiplier;
    surface.ambient_occlusion = saturate(orm.r * mat_data.ao_multiplier);
    surface.roughness = saturate(orm.g * mat_data.roughness_multiplier);
    surface.metallic = saturate(orm.b * mat_data.metallic_multiplier);

    return surface;
}

#endif

#ifdef USE_SELECTION

float4 PSMain(vs_output IN) : SV_TARGET
{
#ifdef USE_ALPHA_CUTOFF
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);

    if (sample_albedo(IN, mat_data).a < mat_data.alpha_cutoff)
        discard;
#endif

    return float4(1.0, 1.0, 1.0, 1.0);
}

#elif defined(WRITE_ID)

uint PSMain(vs_output IN) : SV_TARGET
{
#ifdef USE_ALPHA_CUTOFF
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);

    if (sample_albedo(IN, mat_data).a < mat_data.alpha_cutoff)
        discard;
#endif

    return sfg_constant_obj2;
}

#elif defined(USE_ZPREPASS)

#ifdef USE_ALPHA_CUTOFF
void PSMain(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);

    if (sample_albedo(IN, mat_data).a < mat_data.alpha_cutoff)
        discard;
}
#endif

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
    material_surface surface = sample_material_surface(IN, mat_data);

#ifdef USE_ALPHA_CUTOFF
    if (surface.albedo.a < mat_data.alpha_cutoff)
        discard;
#endif

    ps_output OUT;
    OUT.rt0 = float4(surface.albedo.rgb, 1.0);
    OUT.rt1 = float4(oct_encode(surface.normal), 0.0, 0.0);
    OUT.rt2 = float4(surface.ambient_occlusion, surface.roughness, surface.metallic, 0.0);
    OUT.rt3 = float4(surface.emissive, 1.0);

    return OUT;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    render_pass_data_lighting lighting_data = sfg_get_cbv<render_pass_data_lighting>(SFG_RENDER_PASS_LIGHTING);
    StructuredBuffer<gpu_light> light_buffer = sfg_get_ssbo<gpu_light>(lighting_data.light_buffer_index);
    StructuredBuffer<gpu_shadow_view> shadow_buffer = sfg_get_ssbo<gpu_shadow_view>(lighting_data.shadow_buffer_index);
    StructuredBuffer<gpu_light_cluster> cluster_buffer = sfg_get_ssbo<gpu_light_cluster>(lighting_data.cluster_buffer_index);
    StructuredBuffer<uint> cluster_light_indices = sfg_get_ssbo<uint>(lighting_data.cluster_light_indices_buffer_index);
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    material_surface material = sample_material_surface(IN, mat_data);

#ifdef USE_ALPHA_CUTOFF
    if (material.albedo.a < mat_data.alpha_cutoff)
        discard;
#endif

    float view_depth = abs(mul(view_data.view, float4(IN.world_pos, 1.0)).z);
    uint cluster_index = lighting_data.cluster_buffer_offset + get_light_cluster_index((uint2)IN.pos.xy, view_depth, lighting_data.cluster_dims, lighting_data.cluster_depth);
    gpu_light_cluster cluster = cluster_buffer[cluster_index];

    if (lighting_data.debug_cluster_heatmap != 0)
        return float4(get_light_cluster_heatmap(cluster.light_count, cluster.overflow), material.albedo.a);

    surface_lighting_data surface;
    surface.world_pos = IN.world_pos;
    surface.view_direction = normalize(view_data.camera_pos.xyz - IN.world_pos);
    surface.normal = material.normal;
    surface.albedo = material.albedo.rgb;
    surface.ambient_occlusion = material.ambient_occlusion;
    surface.roughness = material.roughness;
    surface.metallic = material.metallic;

    scene_lighting_data scene;
    scene.view = view_data.view;
    scene.light_counts = lighting_data.light_counts;

    float3 lighting = lighting_data.ambient_color.rgb * material.albedo.rgb * material.ambient_occlusion;
    lighting += evaluate_clustered_scene_lighting(
        surface,
        scene,
        light_buffer,
        shadow_buffer,
        cluster_light_indices,
        cluster.light_offset,
        cluster.light_count,
        smp_shadow,
        smp_shadow_cube);

    return float4(lighting + material.emissive, material.albedo.a);
}

#endif
