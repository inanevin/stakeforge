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

#include "layout_defines.hlsl"

static const float PI = 3.14159265359;
static const uint THREAD_COUNT = 256;
static const uint COEFFICIENT_COUNT = 9;

SamplerState smp_linear : static_sampler_linear;
groupshared float3 coefficient_sums[THREAD_COUNT * COEFFICIENT_COUNT];

float radical_inverse_vdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

[numthreads(THREAD_COUNT, 1, 1)]
void CSMain(uint thread_index : SV_GroupIndex)
{
    // spread uniform sphere samples across the group
    TextureCube<float4> source = sfg_get_texture<TextureCube<float4> >(sfg_constant_rp0);
    RWStructuredBuffer<float4> destination = sfg_get_rws_buffer<float4>(sfg_constant_rp1);
    const uint coefficient_offset = sfg_constant_rp2;
    const uint sample_count = sfg_constant_rp3;
    float3 coefficients[COEFFICIENT_COUNT];

    for (uint coefficient = 0; coefficient < COEFFICIENT_COUNT; ++coefficient)
        coefficients[coefficient] = 0.0;

    for (uint sample_index = thread_index; sample_index < sample_count; sample_index += THREAD_COUNT)
    {
        const float z = 1.0 - 2.0 * ((float(sample_index) + 0.5) / float(sample_count));
        const float radius = sqrt(max(1.0 - z * z, 0.0));
        const float phi = 2.0 * PI * radical_inverse_vdc(sample_index);
        const float3 direction = float3(radius * cos(phi), radius * sin(phi), z);
        const float3 radiance = source.SampleLevel(smp_linear, direction, 0.0).rgb;
        const float basis[COEFFICIENT_COUNT] = {
            0.282095,
            0.488603 * direction.y,
            0.488603 * direction.z,
            0.488603 * direction.x,
            1.092548 * direction.x * direction.y,
            1.092548 * direction.y * direction.z,
            0.315392 * (3.0 * direction.z * direction.z - 1.0),
            1.092548 * direction.x * direction.z,
            0.546274 * (direction.x * direction.x - direction.y * direction.y)
        };

        for (uint coefficient = 0; coefficient < COEFFICIENT_COUNT; ++coefficient)
            coefficients[coefficient] += radiance * basis[coefficient];
    }

    const float integration_weight = 4.0 * PI / float(sample_count);

    for (uint coefficient = 0; coefficient < COEFFICIENT_COUNT; ++coefficient)
        coefficient_sums[coefficient * THREAD_COUNT + thread_index] = coefficients[coefficient] * integration_weight;

    GroupMemoryBarrierWithGroupSync();

    // reduce all partial SH coefficients
    for (uint stride = THREAD_COUNT / 2; stride > 0; stride >>= 1)
    {
        if (thread_index < stride)
        {
            for (uint coefficient = 0; coefficient < COEFFICIENT_COUNT; ++coefficient)
                coefficient_sums[coefficient * THREAD_COUNT + thread_index] += coefficient_sums[coefficient * THREAD_COUNT + thread_index + stride];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    if (thread_index != 0)
        return;

    // apply the cosine convolution and store irradiance
    for (uint coefficient = 0; coefficient < COEFFICIENT_COUNT; ++coefficient)
    {
        float convolution = PI * 0.25;

        if (coefficient == 0)
            convolution = PI;
        else if (coefficient <= 3)
            convolution = 2.0 * PI / 3.0;

        destination[coefficient_offset + coefficient] = float4(coefficient_sums[coefficient * THREAD_COUNT] * convolution, 0.0);
    }
}
