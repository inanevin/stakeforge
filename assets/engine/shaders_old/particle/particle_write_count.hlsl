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
// pass 3 - write dispatch indirect argument count for the count pass.
// ----------------------------------------------
[numthreads(1,1,1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    // dispatch 1,1,1
    uint system_id = dtid.x;

    RWByteAddressBuffer counters = sfg_get_rwb_buffer(sfg_rp_constant0);
    uint alive = counters.Load(4);
    uint count = (alive + 255u) / 256u;

    RWByteAddressBuffer sim_count_indirect_args = sfg_get_rwb_buffer(sfg_rp_constant1);
    sim_count_indirect_args.Store(12, count);
    sim_count_indirect_args.Store(16, 1);
    sim_count_indirect_args.Store(20, 1);

}