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
