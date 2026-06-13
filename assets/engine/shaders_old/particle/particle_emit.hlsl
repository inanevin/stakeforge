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
#include "random.hlsl"
#include "particles.hlsl"
#include "packing_utils.hlsl"

float rand_mm(float val_min, float val_max, uint seed)
{
    return val_min + random01(seed) * (val_max - val_min);
}

float2 rand2(uint seed)
{
    return float2(random01(seed * 1597334677u), random01(seed * 3812015801u));
}

void make_basis(float3 dir, out float3 u, out float3 v)
{
    // pick a helper that's not parallel
    float3 a = (abs(dir.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    u = normalize(cross(a, dir));
    v = cross(dir, u);
}

float3 sample_cone_volume(
    float3 A, float3 B,
    float r0, float r1,
    uint seed)
{
   float3 axis = B - A;
    float L = length(axis);
    if (L <= 1e-6f) return A;

    float3 dir = axis / L;

    float u_t = random01(seed * 747796405u);

    float k = (r1 - r0);
    float t;

    if (abs(k) <= 1e-6f)
    {
        // cylinder (constant radius)
        t = u_t;
    }
    else
    {
        // frustum / cone: sample radius^3 linearly
        float r0_3 = r0 * r0 * r0;
        float r1_3 = r1 * r1 * r1;
        float r_at_t = pow(r0_3 + u_t * (r1_3 - r0_3), 1.0f / 3.0f);
        t = (r_at_t - r0) / k;
    }

    float r_at_t = lerp(r0, r1, t);

    float2 u = rand2(seed * 2891336453u);
    float rr = r_at_t * sqrt(u.x);
    float theta = 6.28318530718f * u.y;

    float3 U, V;
    make_basis(dir, U, V);

    float3 offset = rr * (cos(theta) * U + sin(theta) * V);
    return A + dir * (t * L) + offset;
}

// ----------------------------------------------
// pass 1 - runs 64 threads per system
// emits new particles and appends to alive list b
// ----------------------------------------------
[numthreads(64, 1, 1)] void CSMain(uint3 gtid : SV_GroupThreadID,
                                   uint3 gid : SV_GroupID)
{
    // we dispatch 1, N, 1
    uint system_id = gid.y;

    // early out if system is not emitting.
    StructuredBuffer<uint> emit_counts = sfg_get_ssbo<uint>(sfg_rp_constant1);
    uint emit_count = emit_counts[system_id];
    if(emit_count == 0)
        return;

    RWStructuredBuffer<particle_system_data> system_data = sfg_get_rws_buffer<particle_system_data>(sfg_rp_constant5);
    uint dead = system_data[system_id].dead_count;
    emit_count = min(emit_count, dead);

    // global
    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);

    // per particle
    RWStructuredBuffer<particle_state> states  = sfg_get_rws_buffer<particle_state>(sfg_rp_constant2);
    RWStructuredBuffer<uint> dead_indices = sfg_get_rws_buffer<uint>(sfg_rp_constant3);

    // per system
    StructuredBuffer<particle_emit_args> emit_args     = sfg_get_ssbo<particle_emit_args>(sfg_rp_constant4);
    
    uint max_particles = pass_params.max_particles_per_system;
    uint frame_index = pass_params.frame_index;
    particle_emit_args emit = emit_args[system_id];
    uint offset = max_particles * system_id;
    uint total_capacity = pass_params.max_particles_per_system * pass_params.num_systems;

    RWStructuredBuffer<uint> alive_list_b = sfg_get_rws_buffer<uint>(sfg_rp_constant6);
    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant7);

    // thread 0 emits i=0,64,128,...
    // thread 1 emits i=1,65,129,...
    for (uint i = gtid.x; i < emit_count; i += 64)
    {
        uint old;
        InterlockedAdd(system_data[system_id].dead_count, 0xFFFFFFFFu, old); // -1
        if (old == 0)
        {
            // undo 
            InterlockedAdd(system_data[system_id].dead_count, 1u);
            break;
        }
        uint dead_local_index = old - 1;
        uint emitted_index = dead_indices[offset + dead_local_index];

        // emit state
        uint seed = (system_id * 9781u) ^ (i * 6271u) ^ (frame_index * 15643u);
   
        // write particle state and mark alive.
        particle_state state = (particle_state)0;

        // write position
        float r0 = 0.1f;
        float r1 = emit.cone_radius;

        if(r1 >= 0.1f)
        {
            float3 minp = float3(emit.min_pos_x, emit.min_pos_y, emit.min_pos_z);
            float3 maxp = float3(emit.max_pos_x, emit.max_pos_y, emit.max_pos_z);
            float3 p = sample_cone_volume(minp, maxp, r0, r1, seed * 69069u);
            state.pos_x = p.x;
            state.pos_y = p.y;
            state.pos_z = p.z;
        }
        else{
            state.pos_x = rand_mm(emit.min_pos_x, emit.max_pos_x, seed * 1664545u);
            state.pos_y = rand_mm(emit.min_pos_y, emit.max_pos_y, seed * 1664542u);
            state.pos_z = rand_mm(emit.min_pos_z, emit.max_pos_z, seed * 1664541u);
        }

        // write age - life
        state.age = 0.0f;
        state.lifetime = rand_mm(emit.min_lifetime, emit.max_lifetime, seed * 1261545u);

        // velocity
        state.start_vel_x = rand_mm(emit.min_start_vel_x, emit.max_start_vel_x, seed * 1634515u);
        state.start_vel_y = rand_mm(emit.min_start_vel_y, emit.max_start_vel_y, seed * 16649542u);
        state.start_vel_z = rand_mm(emit.min_start_vel_z, emit.max_start_vel_z, seed * 1668531u);
        if(emit.integrate_points.x >= 0.0f)
        {
            state.mid_vel_x = rand_mm(emit.min_mid_vel_x, emit.max_mid_vel_x, seed * 1624645u);
            state.mid_vel_y = rand_mm(emit.min_mid_vel_y, emit.max_mid_vel_y, seed * 1624742u);
            state.mid_vel_z = rand_mm(emit.min_mid_vel_z, emit.max_mid_vel_z, seed * 1624231u);
            state.end_vel_x = rand_mm(emit.min_end_vel_x, emit.max_end_vel_x, seed * 1684148u);
            state.end_vel_y = rand_mm(emit.min_end_vel_y, emit.max_end_vel_y, seed * 1684412u);
            state.end_vel_z = rand_mm(emit.min_end_vel_z, emit.max_end_vel_z, seed * 1684831u);
        }

        // rotation
        state.rotation = rand_mm(emit.min_start_rotation, emit.max_start_rotation, seed * 1624745u);
        state.start_ang_vel = rand_mm(emit.min_start_angular_velocity, emit.max_start_angular_velocity, seed * 1663585u);
        if(emit.integrate_points.z >= 0.0f)
        {
            state.end_ang_vel = rand_mm(emit.min_end_angular_velocity, emit.max_end_angular_velocity, seed * 2664515u);
        }

        // size & opacity
        float start_size = rand_mm(emit.size_points.x, emit.size_points.y, seed * 4264515u);
        float mid_size = emit.size_points.z;
        float end_size = emit.size_points.w;

        float start_opacity = rand_mm(emit.opacity_points.x, emit.opacity_points.y, seed * 8264541u);
        float mid_opacity = emit.opacity_points.z;
        float end_opacity = emit.opacity_points.w;
        
        state.start_size_opacity = pack_range(float2(start_size, start_opacity), 2.0f);
        state.mid_size_opacity = pack_range(float2(mid_size, mid_opacity), 2.0f);
        state.end_size_opacity = pack_range(float2(end_size, end_opacity), 2.0f);

        // integrate points
        state.size_opacity_integrate_point = pack_range(float2(emit.integrate_points.w, emit.integrate_points.y), 2.0f);
        state.vel_and_ang_vel_integrate_point = pack_range(float2(emit.integrate_points.x, emit.integrate_points.z), 2.0f);

        // color
        float cx = rand_mm(emit.min_col_x, emit.max_col_x, seed * 1684511u);
        float cy = rand_mm(emit.min_col_y, emit.max_col_y, seed * 1682511u);
        float cz = rand_mm(emit.min_col_z, emit.max_col_z, seed * 1484591u);
        state.color = pack_rgba8_unorm(float4(cx, cy, cz, 0.0f));
        state.integrate_point_color = emit.col_integrate_point;

        if(state.integrate_point_color > 0.0f)
        {
            state.mid_color = pack_rgba8_unorm(float4(emit.mid_col_x, emit.mid_col_y, emit.mid_col_z, 0.0f));
            state.end_color = pack_rgba8_unorm(float4(emit.end_col_x, emit.end_col_y, emit.end_col_z, 0.0f));
        }

        // sys
        state.system_id = system_id;

        states[emitted_index] = state;

        // write into alive_list_b.
        uint idx;
        counters.InterlockedAdd(4, 1, idx);
        if (idx < total_capacity) alive_list_b[idx] = emitted_index;
    }
}