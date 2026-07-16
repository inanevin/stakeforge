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

SFG_MATERIAL_TEXTURE("albedo", sfg_texture2d)
SFG_MATERIAL_TEXTURE("emissive", sfg_texture2d)
SFG_MATERIAL_SAMPLER("albedo")
SFG_MATERIAL_SAMPLER("emissive")
SFG_MATERIAL_PARAM_VEC4("base_color_factor", sfg_color)
SFG_MATERIAL_PARAM_VEC4("emissive_factor", sfg_color)
SFG_MATERIAL_PARAM_F32("emissive_strength", 1.0, 0.0, 1000.0)
SFG_MATERIAL_PARAM_F32("alpha_cutoff", 0.5, 0.0, 1.0)
SFG_MATERIAL_PARAM_VEC4("albedo_tiling_offset", sfg_pack_uint2)
SFG_MATERIAL_PARAM_VEC4("emissive_tiling_offset", sfg_pack_uint2)

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
    float3 world_norm : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

#endif

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_opaque rp_data = sfg_get_cbv<render_pass_data_opaque>(sfg_constant_rp0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_constant_rp1);
    gpu_entity entity = entity_buffer[sfg_constant_obj0];

    float4 obj_pos;
#ifndef USE_ZPREPASS
    float3 obj_norm;
#endif

#ifdef USE_SKINNING
    StructuredBuffer<gpu_bone> bone_buffer = sfg_get_ssbo<gpu_bone>(sfg_constant_rp2);
    float4 skinned_pos = float4(0, 0, 0, 0);
#ifndef USE_ZPREPASS
    float3 skinned_normal = 0;
#endif

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index = sfg_constant_obj1 + IN.bone_indices[i];
        float weight = IN.bone_weights[i];
        float4x4 bone_mat = bone_buffer[bone_index].bone;
        skinned_pos += mul(bone_mat, float4(IN.pos, 1.0f)) * weight;
#ifndef USE_ZPREPASS
        skinned_normal += mul((float3x3)bone_mat, IN.normal) * weight;
#endif
    }

    obj_pos = skinned_pos;
#ifndef USE_ZPREPASS
    obj_norm = skinned_normal;
#endif
#else
    obj_pos = float4(IN.pos, 1.0f);
#ifndef USE_ZPREPASS
    obj_norm = IN.normal;
#endif
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.uv = IN.uv;
#ifndef USE_ZPREPASS
    OUT.world_norm = normalize(mul(entity.normal_matrix, float4(obj_norm, 0.0)).xyz);
#endif
    return OUT;
}

struct material_data
{
    float4 base_color_factor;
    float4 emissive_factor;
    float emissive_strength;
    float alpha_cutoff;
    uint2 albedo_tiling_offset;
    uint2 emissive_tiling_offset;
};

#ifdef USE_SELECTION

float4 PSMain(vs_output IN) : SV_TARGET
{
#ifdef USE_ALPHA_CUTOFF
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState sampler_albedo = sfg_get_sampler_state(sfg_constant_mat3);
    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float4 albedo = tex_albedo.Sample(sampler_albedo, IN.uv * albedo_tiling + albedo_offset) * mat_data.base_color_factor;
    if (albedo.a < mat_data.alpha_cutoff)
        discard;
#endif
    return float4(1.0, 1.0, 1.0, 1.0);
}

#elif defined(WRITE_ID)

uint PSMain(vs_output IN) : SV_TARGET
{
#ifdef USE_ALPHA_CUTOFF
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState sampler_albedo = sfg_get_sampler_state(sfg_constant_mat3);
    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float4 albedo = tex_albedo.Sample(sampler_albedo, IN.uv * albedo_tiling + albedo_offset) * mat_data.base_color_factor;
    if (albedo.a < mat_data.alpha_cutoff)
        discard;
#endif
    return sfg_constant_obj2;
}

#else

#if defined(USE_ZPREPASS)

#ifdef USE_ALPHA_CUTOFF
void PSMain(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState sampler_albedo = sfg_get_sampler_state(sfg_constant_mat3);
    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float4 albedo = tex_albedo.Sample(sampler_albedo, IN.uv * albedo_tiling + albedo_offset) * mat_data.base_color_factor;
    if (albedo.a < mat_data.alpha_cutoff)
        discard;
}
#endif

#else

struct ps_output
{
    float4 rt0 : SV_Target0;
    float4 rt1 : SV_Target1;
    float4 rt2 : SV_Target2;
    float4 rt3 : SV_Target3;
};

ps_output PSMain(vs_output IN)
{
    ps_output OUT;

    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    Texture2D tex_emissive = sfg_get_texture<Texture2D>(sfg_constant_mat2);
    SamplerState sampler_albedo = sfg_get_sampler_state(sfg_constant_mat3);
    SamplerState sampler_emissive = sfg_get_sampler_state(sfg_constant_mat4);

    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float2 emissive_tiling = unpack_half2x16(mat_data.emissive_tiling_offset.x);
    float2 emissive_offset = unpack_half2x16(mat_data.emissive_tiling_offset.y);

    float4 albedo = tex_albedo.Sample(sampler_albedo, IN.uv * albedo_tiling + albedo_offset) * mat_data.base_color_factor;
#ifdef USE_ALPHA_CUTOFF
    if (albedo.a < mat_data.alpha_cutoff)
        discard;
#endif

    float3 emissive = tex_emissive.Sample(sampler_emissive, IN.uv * emissive_tiling + emissive_offset).rgb * mat_data.emissive_factor.rgb * mat_data.emissive_strength;
    float2 encoded_normal = oct_encode(normalize(IN.world_norm));

    OUT.rt0 = float4(albedo.rgb, 1.0);
    OUT.rt1 = float4(encoded_normal, 0.0, 0.0);
    OUT.rt2 = float4(1.0, 1.0, 0.0, 1.0);
    OUT.rt3 = float4(albedo.rgb + emissive, 1.0);
    return OUT;
}

#endif

#endif
