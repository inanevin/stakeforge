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