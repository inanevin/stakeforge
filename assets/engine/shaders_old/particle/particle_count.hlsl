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
#include "layout_defines_compute.hlsl"
#include "particles.hlsl"
#include "packing_utils.hlsl"

// ----------------------------------------------
// pass 4 - 1 thread per alive particle
// check visibility, bump instance count and write to instance buffer for
// target system
// ----------------------------------------------
[numthreads(256,1,1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    // we dispatch ceil(alive_count / 256)
    uint alive_index = dtid.x;

    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant1);
    uint aliveCount = counters.Load(4); 

    if (alive_index >= aliveCount) return;

    RWStructuredBuffer<uint> alive_list_b = sfg_get_rws_buffer<uint>(sfg_rp_constant2);
    uint particle_index = alive_list_b[alive_index];

    RWStructuredBuffer<particle_state> states  = sfg_get_rws_buffer<particle_state>(sfg_rp_constant3);
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    
    // simple cull, no render if behind camera.
    float3 particle_pos = float3(states[particle_index].pos_x, states[particle_index].pos_y, states[particle_index].pos_z);
    float3 cam_to_particle = particle_pos - pass_params.cam_pos_and_delta.xyz;
    float dp = dot(pass_params.cam_dir.xyz, cam_to_particle);
    uint visible = dp < 0.0 ? 0 : 1;

    if(visible == 0)
        return;

    // sort_keys[particle_index] = ~asuint(dot(cam_to_particle, cam_to_particle));

    uint system_id = states[particle_index].system_id;
    uint start = system_id * pass_params.max_particles_per_system;

    RWStructuredBuffer<particle_indirect_args> indirect_args     = sfg_get_rws_buffer<particle_indirect_args>(sfg_rp_constant4);
    uint idx;
    InterlockedAdd(indirect_args[system_id].instance_count, 1, idx);
    if (idx >= pass_params.max_particles_per_system) return;

    RWStructuredBuffer<particle_instance_data> instance_data    = sfg_get_rws_buffer<particle_instance_data>(sfg_rp_constant5);

    float age_ratio = states[particle_index].age / states[particle_index].lifetime;

    float3 position = float3(states[particle_index].pos_x, states[particle_index].pos_y, states[particle_index].pos_z);
    float3 vel = float3(states[particle_index].vel_x, states[particle_index].vel_y, states[particle_index].vel_z);
    float rotation = states[particle_index].rotation;

    float2 start_size_opacity = unpack_range(states[particle_index].start_size_opacity, 2.0f);
    float2 size_opacity_integrate_points = unpack_range(states[particle_index].size_opacity_integrate_point, 2.0f);
    float2 mid_size_opacity = unpack_range(states[particle_index].mid_size_opacity, 2.0f);
    float2 end_size_opacity = unpack_range(states[particle_index].end_size_opacity, 2.0f);

    // find size
    float size = start_size_opacity.x;
    float size_integ = size_opacity_integrate_points.x;
    if(size_integ > 0.0f)
    {
        if(age_ratio < size_integ)
        {
            size = lerp(size, mid_size_opacity.x, age_ratio / size_integ);
        }
        else
        {
            size = lerp(mid_size_opacity.x, end_size_opacity.x, (age_ratio - size_integ) / (1.0 - size_integ));
        }
    }

    // find opacity
    float opacity = start_size_opacity.y;
    float opacity_integ = size_opacity_integrate_points.y;
    if(opacity_integ > 0.0f)
    {
        if(age_ratio < opacity_integ)
        {
            opacity = lerp(opacity, mid_size_opacity.y, age_ratio / opacity_integ);
        }
        else
        {
            opacity = lerp(mid_size_opacity.y, end_size_opacity.y, (age_ratio - opacity_integ) / (1.0 - opacity_integ));
        }
    }
    
    // color
    float4 col = unpack_rgba8_unorm(states[particle_index].color);
    float col_integ = states[particle_index].integrate_point_color;
    if(col_integ > 0.0f)
    {
        float4 mid = unpack_rgba8_unorm(states[particle_index].mid_color);
        float4 end = unpack_rgba8_unorm(states[particle_index].end_color);
        if(age_ratio < col_integ)
        {
            col = lerp(col, mid, age_ratio / col_integ);
        }
        else{
            col = lerp(mid, end, (age_ratio - col_integ) / (1.0f - col_integ));
        }
    }
    col.w = opacity;

    particle_instance_data pid = (particle_instance_data)0;
    pid.pos_rot_size = float4(position.x, position.y, position.z, pack_rot_size(rotation, size));
    pid.velocity = float4(vel.x, vel.y, vel.z, 0.0f);
    pid.color = pack_rgba8_unorm(col);

    instance_data[start + idx] = pid;
}