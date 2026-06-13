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

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD4;
};

//------------------------------------------------------------------------------
// Vertex Shader
//------------------------------------------------------------------------------

vs_output VSMain(vs_input IN)
{
    vs_output OUT;

    render_pass_data_forward rp_data = sfg_get_cbv<render_pass_data_forward>(sfg_rp_constant0);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(sfg_rp_constant1);
    gpu_entity entity = entity_buffer[sfg_object_constant0];

    float4 obj_pos;

#ifdef USE_SKINNING

    StructuredBuffer<gpu_bone> bone_buffer =  sfg_get_ssbo<gpu_bone>(sfg_rp_constant2);

    // skinning in object space
    float4 skinned_pos    = float4(0, 0, 0, 0);

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

//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------


struct material_data
{
    float4 base_color_factor;
    uint2 albedo_tiling_offset;
};

struct texture_data
{
    uint gpu_index_albedo;
};

#ifdef WRITE_ID

uint PSMain(vs_output IN) : SV_TARGET
{
    return sfg_object_constant2;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_forward params = sfg_get_cbv<render_pass_data_forward>(sfg_rp_constant0);

    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    texture_data txt_data = sfg_get_cbv<texture_data>(sfg_mat_constant1);
    Texture2D tex_albedo = sfg_get_texture<Texture2D>(txt_data.gpu_index_albedo);
    SamplerState sampler_default = sfg_get_sampler_state(sfg_mat_constant2);

    float2 albedo_tiling = unpack_half2x16(mat_data.albedo_tiling_offset.x);
    float2 albedo_offset = unpack_half2x16(mat_data.albedo_tiling_offset.y);
    float2 albedo_uv = IN.uv * albedo_tiling + albedo_offset;

    // --- Base color ---
    float4 albedo_tex = tex_albedo.Sample(sampler_default, albedo_uv);
    float4 albedo = albedo_tex * mat_data.base_color_factor * params.ambient ;

    return albedo;
}

#endif

