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
