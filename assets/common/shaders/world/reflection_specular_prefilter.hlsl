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

SamplerState smp_linear : static_sampler_linear;

float radical_inverse_vdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 hammersley(uint index, uint count)
{
    return float2(float(index) / float(count), radical_inverse_vdc(index));
}

float3 cubemap_direction(uint face, float2 uv)
{
    const float2 p = uv * 2.0 - 1.0;

    if (face == 0)
        return normalize(float3(1.0, -p.y, -p.x));
    if (face == 1)
        return normalize(float3(-1.0, -p.y, p.x));
    if (face == 2)
        return normalize(float3(p.x, 1.0, p.y));
    if (face == 3)
        return normalize(float3(p.x, -1.0, -p.y));
    if (face == 4)
        return normalize(float3(p.x, -p.y, 1.0));

    return normalize(float3(-p.x, -p.y, -1.0));
}

float3 importance_sample_ggx(float2 xi, float3 normal, float roughness)
{
    const float alpha = roughness * roughness;
    const float phi = 2.0 * PI * xi.x;
    const float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
    const float sin_theta = sqrt(max(1.0 - cos_theta * cos_theta, 0.0));
    const float3 half_tangent = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
    const float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 tangent = normalize(cross(up, normal));
    const float3 bitangent = cross(normal, tangent);

    return normalize(tangent * half_tangent.x + bitangent * half_tangent.y + normal * half_tangent.z);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatch_id : SV_DispatchThreadID)
{
    // fetch this mip's filtering parameters
    const uint destination_size = sfg_constant_rp2;
    const uint sample_count = sfg_constant_rp3;
    const float roughness = asfloat(sfg_constant_rp4);
    const uint source_size = sfg_constant_rp5;

    if (dispatch_id.x >= destination_size || dispatch_id.y >= destination_size || dispatch_id.z >= 6)
        return;

    // map this thread to its cubemap direction
    TextureCube<float4> source = sfg_get_texture<TextureCube<float4> >(sfg_constant_rp0);
    RWTexture2DArray<float4> destination = sfg_get_texture<RWTexture2DArray<float4> >(sfg_constant_rp1);
    const float2 uv = (float2(dispatch_id.xy) + 0.5) / float(destination_size);
    const float3 normal = cubemap_direction(dispatch_id.z, uv);
    const float3 view = normal;
    float3 color = 0.0;
    float weight = 0.0;

    // integrate the GGX lobe
    for (uint i = 0; i < sample_count; ++i)
    {
        const float3 half_vector = importance_sample_ggx(hammersley(i, sample_count), normal, roughness);
        const float3 light = normalize(2.0 * dot(view, half_vector) * half_vector - view);
        const float ndotl = saturate(dot(normal, light));

        if (ndotl <= 0.0)
            continue;

        const float ndoth = saturate(dot(normal, half_vector));
        const float hdotv = saturate(dot(half_vector, view));
        const float alpha = roughness * roughness;
        const float alpha_sq = alpha * alpha;
        const float denominator = ndoth * ndoth * (alpha_sq - 1.0) + 1.0;
        const float distribution = alpha_sq / max(PI * denominator * denominator, 1e-6);
        const float pdf = max(distribution * ndoth / max(4.0 * hdotv, 1e-6), 1e-6);
        const float texel_solid_angle = 4.0 * PI / (6.0 * float(source_size * source_size));
        const float sample_solid_angle = 1.0 / (float(sample_count) * pdf);
        const float mip_level = roughness <= 0.0001 ? 0.0 : 0.5 * log2(sample_solid_angle / texel_solid_angle);

        color += source.SampleLevel(smp_linear, light, mip_level).rgb * ndotl;
        weight += ndotl;
    }

    // write this face and mip
    destination[dispatch_id] = float4(color / max(weight, 1e-5), 1.0);
}
