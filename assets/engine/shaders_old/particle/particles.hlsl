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

struct particle_pass_data
{
    float4x4 view_proj;
    float4 cam_pos_and_delta;
    float4 cam_dir;
    uint max_particles_per_system;
    uint frame_index;
    uint max_systems;
    uint num_systems;
};

struct particle_system_data
{
    uint alive_count;
    uint dead_count;
};


struct particle_emit_args
{
    float4 integrate_points; // vel, op, ang, sz
    float4 opacity_points;	  // min_start, max_start, mid, end
    float4 size_points;	  // min_start, max_start, mid, end

    float min_lifetime;
    float max_lifetime;

    float min_pos_x;
    float min_pos_y;
    float min_pos_z;
    float max_pos_x;
    float max_pos_y;
    float max_pos_z;
    float cone_radius;

    float min_start_vel_x;
    float min_start_vel_y;
    float min_start_vel_z;
    float max_start_vel_x;
    float max_start_vel_y;
    float max_start_vel_z;

    float min_mid_vel_x;
    float min_mid_vel_y;
    float min_mid_vel_z;
    float max_mid_vel_x;
    float max_mid_vel_y;
    float max_mid_vel_z;

    float min_end_vel_x;
    float min_end_vel_y;
    float min_end_vel_z;
    float max_end_vel_x;
    float max_end_vel_y;
    float max_end_vel_z;

    float min_col_x;
    float min_col_y;
    float min_col_z;
    float max_col_x;
    float max_col_y;
    float max_col_z;

    float mid_col_x;
	float mid_col_y;
	float mid_col_z;
	float end_col_x;
	float end_col_y;
	float end_col_z;
	float col_integrate_point;

    float min_start_rotation;
    float max_start_rotation;
    float min_start_angular_velocity;
    float max_start_angular_velocity;
    float min_end_angular_velocity;
    float max_end_angular_velocity;
};

struct particle_state
{
    float pos_x;
    float pos_y;
    float pos_z;

    float age;
    float lifetime;

    float start_vel_x;
    float start_vel_y;
    float start_vel_z;
    float mid_vel_x;
    float mid_vel_y;
    float mid_vel_z;
    float end_vel_x;
    float end_vel_y;
    float end_vel_z;
    float vel_x;
    float vel_y;
    float vel_z;

    float rotation;
    float start_ang_vel;
    float end_ang_vel;

    uint start_size_opacity;
    uint mid_size_opacity;
    uint end_size_opacity;
    uint size_opacity_integrate_point;
    uint vel_and_ang_vel_integrate_point;

    uint color;
    uint mid_color;
    uint end_color;
    float integrate_point_color;
    
    uint system_id;
};

struct particle_instance_data
{
    float4 pos_rot_size;
    float4 velocity;
    uint color;
};

struct particle_counters
{
    uint alive_count_a;
    uint alive_count_b;
};

struct particle_indirect_args
{
    uint vertex_count;
    uint instance_count;
    uint start_vertex;
    uint start_instance;
};

struct particle_sim_count_args
{
    uint group_sim_x;
    uint group_sim_y;
    uint group_sim_z;
    uint group_count_x;
    uint group_count_y;
    uint group_count_z;
};

float2 unpack_rot_size(uint val)
{
    const float TWO_PI = 6.283185307179586f;
    uint lo = val & 0xFFFFu;
    float rad = ((float)lo / 65535.0f) * TWO_PI;

    float max_size_range = 2.0f;
    uint hi = val >> 16;
    float size = (float)hi * (max_size_range / 65535.0f);

    return float2(rad, size);
}

uint pack_rot_size(float rot, float size)
{
    const float TWO_PI = 6.283185307179586f;
    float max_size_range = 2.0f;
    float s = saturate(size / max_size_range);
    uint q = (uint)round(s * 65535.0f);
    q = min(q, 65535u);

    float phase = frac(rot / TWO_PI);
    uint lo = (uint)round(saturate(phase) * 65535.0f) & 0xFFFFu;
    return lo | (q << 16);
}
