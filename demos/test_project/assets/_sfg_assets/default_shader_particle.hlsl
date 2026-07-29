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
#include "render_pass_defines.hlsl"
#include "fog.hlsl"

SFG_MATERIAL_TEXTURE("sprite", sfg_sprite)
SFG_MATERIAL_SAMPLER("sprite")
SFG_MATERIAL_PARAM_VEC4("tint", sfg_color_hdr)

struct particle_instance
{
    float3 position;
    float rotation;
    float3 velocity;
    float size;
    float4 color;
};

struct material_data
{
    float4 tint;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 world_pos : TEXCOORD1;
    float4 color : COLOR0;
};

float2 rotate_particle_position(float2 position, float angle)
{
    float sine;
    float cosine;
    sincos(angle, sine, cosine);
    return float2(position.x * cosine - position.y * sine, position.x * sine + position.y * cosine);
}

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
    StructuredBuffer<particle_instance> instance_buffer = sfg_get_ssbo<particle_instance>(sfg_constant_obj0);
    particle_instance instance = instance_buffer[sfg_constant_obj1 + instance_id];

    float rotation = radians(instance.rotation);
    float3 axis_x = normalize(mul(view_data.inv_view, float4(1.0, 0.0, 0.0, 0.0)).xyz);
    float3 axis_y = normalize(mul(view_data.inv_view, float4(0.0, 1.0, 0.0, 0.0)).xyz);

    if (sfg_constant_obj2 == 1)
    {
        float2 view_velocity = mul(view_data.view, float4(instance.velocity, 0.0)).xy;

        if (dot(view_velocity, view_velocity) > 0.000001)
            rotation += atan2(view_velocity.x, view_velocity.y);
    }
    else if (sfg_constant_obj2 == 2)
    {
        axis_y = float3(0.0, 1.0, 0.0);
        float3 to_camera = view_data.camera_pos.xyz - instance.position;
        to_camera.y = 0.0;

        if (dot(to_camera, to_camera) > 0.000001)
            axis_x = normalize(cross(axis_y, normalize(to_camera)));
    }
    else if (sfg_constant_obj2 == 3)
    {
        axis_x = float3(1.0, 0.0, 0.0);
        axis_y = float3(0.0, 0.0, -1.0);
    }

    float2 local_position = rotate_particle_position(positions[vertex_id], rotation);
    local_position *= float2(instance.size * asfloat(sfg_constant_obj8), instance.size);
    float3 world_position = instance.position + axis_x * local_position.x + axis_y * local_position.y;
    float2 uv_start = float2(asfloat(sfg_constant_obj4), asfloat(sfg_constant_obj5));
    float2 uv_size = float2(asfloat(sfg_constant_obj6), asfloat(sfg_constant_obj7));

    vs_output OUT;
    OUT.pos = mul(view_data.view_proj, float4(world_position, 1.0));
    OUT.uv = uv_start + uvs[vertex_id] * uv_size;
    OUT.world_pos = world_position;
    OUT.color = instance.color;
    return OUT;
}

float4 sample_particle(vs_output IN)
{
    material_data mat_data = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D particle_sprite = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState particle_sampler = sfg_get_sampler_state(sfg_constant_mat2);
    float4 color = particle_sprite.Sample(particle_sampler, IN.uv) * IN.color * mat_data.tint;
    clip(color.a - 0.001);

#ifdef PARTICLE_PREMULTIPLIED_ALPHA
    color.rgb *= color.a;
#endif

    return color;
}

#ifdef USE_SELECTION

float4 PSMain(vs_output IN) : SV_TARGET
{
    sample_particle(IN);
    return float4(1.0, 1.0, 1.0, 1.0);
}

#elif defined(WRITE_ID)

uint PSMain(vs_output IN) : SV_TARGET
{
    sample_particle(IN);
    return sfg_constant_obj3;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    render_pass_data_fog fog_data = sfg_get_cbv<render_pass_data_fog>(SFG_RENDER_PASS_FOG);
    float4 color = sample_particle(IN);

    if ((view_data.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG) != 0)
        color.rgb = apply_fog(color.rgb, IN.world_pos, view_data.camera_pos.xyz, fog_data);

    return color;
}

#endif
