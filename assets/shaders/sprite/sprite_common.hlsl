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
#include "normal.hlsl"
#include "render_pass_defines.hlsl"
#include "fog.hlsl"

SamplerState smp_linear : static_sampler_linear;
SamplerState smp_nearest : static_sampler_nearest;

struct sprite_instance
{
    float2 uv_start;
    float2 uv_size;
    float2 size;
    uint texture_index;
    uint entity_index;
    uint entity_id;
    uint is_linear_sample;
};

struct material_data
{
    float4 tint;
    float alpha_cutoff;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 world_pos : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    nointerpolation uint texture_index : TEXCOORD3;
    nointerpolation uint entity_id : TEXCOORD4;
    nointerpolation uint is_linear_sample : TEXCOORD5;
};

vs_output VSMain(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    static const float2 positions[6] = {
        float2(-0.5, -0.5),
        float2(-0.5, 0.5),
        float2(0.5, 0.5),
        float2(-0.5, -0.5),
        float2(0.5, 0.5),
        float2(0.5, -0.5)
    };
    static const float2 uvs[6] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0)
    };

    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    StructuredBuffer<gpu_entity> entity_buffer = sfg_get_ssbo<gpu_entity>(SFG_RENDER_PASS_ENTITIES);
    StructuredBuffer<sprite_instance> instance_buffer = sfg_get_ssbo<sprite_instance>(sfg_constant_obj0);
    sprite_instance instance = instance_buffer[sfg_constant_obj1 + instance_id];
    gpu_entity entity = entity_buffer[instance.entity_index];

    float2 local_position = positions[vertex_id] * instance.size;
    float3 world_position = mul(entity.model, float4(local_position, 0.0, 1.0)).xyz;

    vs_output OUT;
    OUT.pos = mul(view_data.view_proj, float4(world_position, 1.0));
    OUT.uv = instance.uv_start + uvs[vertex_id] * instance.uv_size;
    OUT.world_pos = world_position;
    OUT.world_normal = normalize(mul(entity.normal_matrix, float4(0.0, 0.0, 1.0, 0.0)).xyz);
    OUT.texture_index = instance.texture_index;
    OUT.entity_id = instance.entity_id;
    OUT.is_linear_sample = instance.is_linear_sample;
    return OUT;
}

float4 sample_sprite(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D texture_sprite = sfg_get_texture_non_uniform<Texture2D>(IN.texture_index);
    float4 color = IN.is_linear_sample != 0 ? texture_sprite.Sample(smp_linear, IN.uv) : texture_sprite.Sample(smp_nearest, IN.uv);
    color *= mat_data.tint;
    clip(color.a - 0.001);
    return color;
}
