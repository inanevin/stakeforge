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
#include "packing_utils.hlsl"
#include "entity.hlsl"
#include "normal.hlsl"
#include "bone.hlsl"
#include "render_pass_defines.hlsl"

//------------------------------------------------------------------------------
// In & Outs
//------------------------------------------------------------------------------

struct vs_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

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

    float3 gizmo_world_pos = entity.model[3].xyz;
    float depth = length(rp_data.camera_pos.xyz - gizmo_world_pos);

    // Projection params
    float H = rp_data.resolution.y;
    float tanHalfFovY = rp_data.proj_params.x;
    float world_scale = (2.0f * tanHalfFovY * depth) * (70 / H) ;
    //world_scale = clamp(world_scale, 0.001f, 1e6f);

    // Apply scale in object space around the gizmo origin:
    float3 scaledLocal = IN.pos * world_scale;

    // Transform to world and clip
    float4 objPos = float4(scaledLocal, 1.0f);
    float3 worldPos = mul(entity.model, objPos).xyz;
    OUT.pos = mul(rp_data.view_proj, float4(worldPos, 1.0f));
    OUT.uv  = IN.uv;
    return OUT;
}

//------------------------------------------------------------------------------
// Pixel Shader (G-Buffer)
//------------------------------------------------------------------------------

struct material_data
{
    float4 color;
};

#ifdef WRITE_ID

uint PSMain(vs_output IN) : SV_TARGET
{
    return sfg_object_constant1;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_mat_constant0);
    return mat_data.color;
}

#endif