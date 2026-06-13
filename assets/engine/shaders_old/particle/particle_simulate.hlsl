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
// pass 2 - 1 thread per alive particle.abort
// simulate each particle in alive_list_a, append to dead list if dead
// append to alive_list_b if survives.
// ----------------------------------------------
[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID)
{
    // will be dispatched total_alive_count / 256, 1, 1
    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant3);
    uint thread_index = dtid.x;

    // initial run, 1 thread 0 alive.
    if(thread_index >= counters.Load(0))
        return;
    
    RWStructuredBuffer<uint> alive_list_a = sfg_get_rws_buffer<uint>(sfg_rp_constant1);
    uint particle_index = alive_list_a[thread_index];

    RWStructuredBuffer<particle_state> states  = sfg_get_rws_buffer<particle_state>(sfg_rp_constant4);
    particle_state state = states[particle_index];
    
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    float delta = pass_params.cam_pos_and_delta.w;
    float age = state.age;
    float lifetime = state.lifetime;
    age += delta;

    if(age > lifetime)
    {
        uint system_id = state.system_id;
        uint old_dead_count;

        // increment system's dead count.
        RWStructuredBuffer<particle_system_data> system_data = sfg_get_rws_buffer<particle_system_data>(sfg_rp_constant5);
        InterlockedAdd(system_data[system_id].dead_count, 1, old_dead_count);

        // find global dead index from local old_dead_count
        uint max_particles = pass_params.max_particles_per_system;
        uint global_dead_index = system_id * max_particles + old_dead_count;

        // mark it as dead.
        RWStructuredBuffer<uint> dead_indices = sfg_get_rws_buffer<uint>(sfg_rp_constant6);

        if (old_dead_count < max_particles)
            dead_indices[global_dead_index] = particle_index;

        return;
    }

    // integrate
    float life_ratio = age / lifetime;

    // calculate current velocity.
    float3 vel = float3(state.start_vel_x, state.start_vel_y, state.start_vel_z);
    float2 vel_and_ang_vel_integs = unpack_range(state.vel_and_ang_vel_integrate_point, 2.0f);
    float vel_integ = vel_and_ang_vel_integs.x;
    if(vel_integ > 0.0f)
    {
        float3 mid_vel = float3(state.mid_vel_x, state.mid_vel_y, state.mid_vel_z);
        float3 end_vel = float3(state.end_vel_x, state.end_vel_y, state.end_vel_z);
        if(life_ratio < vel_integ)
        {
            vel = lerp(vel, mid_vel, life_ratio / vel_integ);
        }
        else
        {
            float t = (life_ratio - vel_integ) / (1.0f - vel_integ);
            vel = lerp(mid_vel, end_vel, t);
        }
    }

    // integrate current pos.
    float3 pos = float3(state.pos_x, state.pos_y, state.pos_z);
    pos += vel * delta;

    // calculate current angular velocity
    float ang_vel_integ = vel_and_ang_vel_integs.y;
    float ang_vel = state.start_ang_vel;
    if(ang_vel_integ > 0.0)
    {
        ang_vel = lerp(ang_vel, state.end_ang_vel, saturate(life_ratio / ang_vel_integ));
    }

    // integrate current rot
    float rot = state.rotation;
    rot += ang_vel * delta;

    // store.
    states[particle_index].age = age;
    states[particle_index].pos_x = pos.x;
    states[particle_index].pos_y = pos.y;
    states[particle_index].pos_z = pos.z;
    states[particle_index].vel_x = vel.x;
    states[particle_index].vel_y = vel.y;
    states[particle_index].vel_z = vel.z;
    states[particle_index].rotation = rot;

    // write into alive_count_b.
    uint idx;
    counters.InterlockedAdd(4, 1, idx);
    RWStructuredBuffer<uint> alive_list_b = sfg_get_rws_buffer<uint>(sfg_rp_constant2);
    alive_list_b[idx] = particle_index;

}