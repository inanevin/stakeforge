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

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

#ifdef USE_SKINNING

struct vs_input
{
	float3 pos : POSITION;
	float3 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float2 uv : TEXCOORD0;
    float4 bone_weights : BLENDWEIGHT0;
    uint4  bone_indices : BLENDINDICES0;
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
    float3 world_tan : TEXCOORD1;
    float3 world_bit : TEXCOORD2;
    float2 uv : TEXCOORD4;
};

#endif


//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

#ifdef USE_ZPREPASS

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_opaque rp_data = sfg_get_cbv<render_pass_data_opaque>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);

    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;

#ifdef USE_SKINNING
    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);

    // skinning in object space
    float4 skinned_pos    = float4(0, 0, 0, 0);
    float3 skinned_normal = 0;
    float3 skinned_tan    = 0;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index   = sfg_object_constant1 + IN.bone_indices[i];
        float weight      = IN.bone_weights[i];
        float4x4 bone_mat = bone_buffer[bone_index].bone;
        skinned_pos += mul(bone_mat, float4(IN.pos, 1.0f)) * weight;
    }

    obj_pos  = skinned_pos;
#else
    obj_pos = float4(IN.pos, 1.0f);
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.uv = IN.uv;
    return OUT;
}

#else

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_opaque rp_data = sfg_get_cbv<render_pass_data_opaque>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;
    float3 obj_norm;
    float3 obj_tan;

#ifdef USE_SKINNING

    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);

    // skinning in object space
    float4 skinned_pos    = float4(0, 0, 0, 0);
    float3 skinned_normal = 0;
    float3 skinned_tan    = 0;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        uint bone_index   = sfg_object_constant1 + IN.bone_indices[i];
        float weight      = IN.bone_weights[i];
        float4x4 bone_mat = bone_buffer[bone_index].bone;

        skinned_pos    += mul(bone_mat, float4(IN.pos, 1.0f)) * weight;
        skinned_normal += mul((float3x3)bone_mat, IN.normal) * weight;
        skinned_tan    += mul((float3x3)bone_mat, IN.tangent.xyz) * weight;
    }

    obj_pos  = skinned_pos;
    obj_norm = skinned_normal;
    obj_tan  = skinned_tan;
#else
    obj_pos = float4(IN.pos, 1.0f);
    obj_norm = IN.normal;
    obj_tan = IN.tangent.xyz;
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;
    float3 N = normalize(mul(entity.normal_matrix, float4(obj_norm, 1.0)).xyz);
    float3 T = normalize(mul(entity.normal_matrix, float4(obj_tan, 1.0)).xyz);

    // gram-schmidt ensure orthogonality.
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T)) * IN.tangent.w;

    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    OUT.world_norm = N;
    OUT.world_tan = T;
    OUT.world_bit = B;
    OUT.uv = IN.uv;

    return OUT;
}

#endif


//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------


struct material_data
{
    float4 base_color_factor;
    float4 emissive_and_metallic_factor;
    float4 roughness_normal_strength_alpha;
    uint2 albedo_tiling_offset;
    uint2 normal_tiling_offset;
    uint2 orm_tiling_offset;
    uint2 emissive_tiling_offset;
};

struct texture_data
{
    uint gpu_index_albedo;
    uint gpu_index_normal;
    uint gpu_index_orm;
    uint gpu_index_emissive;
};

#ifdef WRITE_ID

uint PSMain(vs_output IN) : SV_TARGET
{
    return sfg_object_constant2;
}

#else

#if defined(USE_ZPREPASS) 

#ifdef USE_ALPHA_CUTOFF
void PSMain(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float4 albedo_tex = tex_albedo.Sample(sampler_default, IN.uv);
	if(albedo_tex.a < mat_data.roughness_normal_strength_alpha.z)
	{
		discard;
	}
}
#endif

#else

struct ps_output
{
    float4 rt0 : SV_Target0; // Albedo
    float4 rt1 : SV_Target1; // World Normal
    float4 rt2 : SV_Target2; // orm
    float4 rt3 : SV_Target3; // Emissive
};

ps_output PSMain(vs_output IN)
{
    
    ps_output OUT;

    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    Texture2D tex_normal = sfg_get_texture<Texture2D>(txt_data.gpu_index_normal);
    Texture2D tex_orm = sfg_get_texture<Texture2D>(txt_data.gpu_index_orm);
    Texture2D tex_emissive = sfg_get_texture<Texture2D>(txt_data.gpu_index_emissive);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float2 albedo_uv = IN.uv * albedo_tiling + albedo_offset;

    float2 normal_tiling = unpack_half2x16(mat_data.normal_tiling_offset.x);
    float2 normal_offset = unpack_half2x16(mat_data.normal_tiling_offset.y);
    float2 normal_uv = IN.uv * normal_tiling + normal_offset;

    float2 orm_tiling = unpack_half2x16(mat_data.orm_tiling_offset.x);
    float2 orm_offset = unpack_half2x16(mat_data.orm_tiling_offset.y);
    float2 orm_uv = IN.uv * orm_tiling + orm_offset;

    float2 emissive_tiling = unpack_half2x16(mat_data.emissive_tiling_offset.x);
    float2 emissive_offset = unpack_half2x16(mat_data.emissive_tiling_offset.y);
    float2 emissive_uv = IN.uv * emissive_tiling + emissive_offset;

    // --- Base color ---
    float4 albedo_tex = tex_albedo.Sample(sampler_default, albedo_uv);
    float4 albedo = albedo_tex * mat_data.base_color_factor;
#ifdef USE_ALPHA_CUTOFF
	if(albedo.a < mat_data.roughness_normal_strength_alpha.z)
	{
		discard;
	}
#endif

    // normal, convert to -1, 1 vector 
    float3 tangent_normal = tex_normal.Sample(sampler_default, normal_uv).xyz * 2.0 - 1.0;
    float s = mat_data.roughness_normal_strength_alpha.y;
    float2 xy = tangent_normal.xy * s;
    float z = sqrt(saturate(1.0 - dot(xy, xy)));
    tangent_normal = float3(xy, z);
    //tangent_normal = normalize(tangent_normal);

    // convert normal to world space normal, [-1, 1]
    float3x3 TBN = float3x3(IN.world_tan, IN.world_bit, IN.world_norm);
    float3 world_normal = normalize(mul(tangent_normal, TBN));
    float2 encoded_normal = oct_encode(world_normal);

    // --- orm ---
    float3 orm_tex = tex_orm.Sample(sampler_default, orm_uv).rgb;
    float ao = saturate(orm_tex.r);
    float roughness = saturate(orm_tex.g * mat_data.roughness_normal_strength_alpha.x);
    float metallic = saturate(orm_tex.b * mat_data.emissive_and_metallic_factor.w);

    // emissive
    float3 emissive_tex = tex_emissive.Sample(sampler_default, emissive_uv).rgb;
    float3 emissive = emissive_tex * mat_data.emissive_and_metallic_factor.xyz * 3;

    // outs
    OUT.rt0 = float4(albedo.xyz, 1.0);
    OUT.rt1 = float4(encoded_normal, 0.0, 0.0);
    OUT.rt2 = float4(ao, roughness, metallic, 1.0);
    OUT.rt3 = float4(emissive, 1.0);

    return OUT;
}

#endif

#endif