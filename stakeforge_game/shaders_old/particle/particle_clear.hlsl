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
#include "layout_defines_compute.hlsl"
#include "particles.hlsl"

// ----------------------------------------------
// pass 0 - reset each system's per frame data
// ----------------------------------------------
[numthreads(64,1,1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    // dispatch ceil(num systems / 64)
    uint system_id = dtid.x;

    ConstantBuffer<particle_pass_data> pass_params = sfg_get_cbv<particle_pass_data>(sfg_rp_constant0);
    if (system_id >= pass_params.num_systems) return;

    RWStructuredBuffer<particle_indirect_args> indirect_args     = sfg_get_rws_buffer<particle_indirect_args>(sfg_rp_constant1);
    indirect_args[system_id].vertex_count = 4;
    indirect_args[system_id].start_vertex = 0;
    indirect_args[system_id].instance_count = 0;
    indirect_args[system_id].start_instance = system_id * pass_params.max_particles_per_system;
}