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
#include "layout_defines.hlsl"
#include "entity.hlsl"
#include "bone.hlsl"

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
#ifdef USE_SKINNING
    float4 bone_weights : BLENDWEIGHT0;
    uint4  bone_indices : BLENDINDICES0;
#endif
};

struct vs_output
{
    float4 pos : SV_POSITION;
};

struct render_pass_data
{
    float4x4 view_proj;
};

vs_output VSMain(vs_input IN)
{
    vs_output OUT;
    
    render_pass_data rp_data = sfg_get_cbv<render_pass_data>(sfg_rp_constant0);
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
        skinned_pos    += mul(bone_mat, float4(IN.pos, 1.0f)) * weight;
    }

    obj_pos  = skinned_pos;
#else
    obj_pos = float4(IN.pos, 1.0f);
#endif

    float3 world_pos = mul(entity.model, obj_pos).xyz;

    OUT.pos = mul(rp_data.view_proj, float4(world_pos, 1.0f));
    return OUT;
}

float4 PSMain(vs_output IN) : SV_Target0
{
    return float4(1.0, 1.0, 1.0, 1.0);
}

