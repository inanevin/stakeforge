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

float linearize_depth(float depth, float nearZ, float farZ) 
{
    float z = (1.0 - depth) * 2.0 - 1.0; // reverse z flip
    return (2.0 * nearZ * farZ) / (farZ + nearZ - z * (farZ - nearZ));
}

static float3 reconstruct_world_position(float2 uv, float device_depth, float4x4 inv_view_proj)
{
    // NDC y is Y up, uv Y is Y down.
    float2 ndcXY = float2(uv.x * 2.0f - 1.0f,
                      1.0f - uv.y * 2.0f);
                      
    // depth is 0-1 already DX NDC.
    float  ndcZ  = device_depth;
    
    float4 ndc = float4(ndcXY, ndcZ, 1.0);

    // in world space
    ndc = mul(inv_view_proj, ndc);
    ndc /= ndc.w;

    return ndc.xyz;
}

// Very small epsilon guard
static bool is_background(float device_depth)
{
    // Reversed-Z: far = 0, near = 1. Treat ~0 as background
    return device_depth <= 1e-6;
}
