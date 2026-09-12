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

static const float PI = 3.14159265359;

float distribution_ggx(float NdotH, float roughness)
{
    float a  = max(roughness * roughness, 1e-4); // α
    float a2 = a * a;
    float nh = saturate(NdotH);
    float nh2 = nh * nh;

    float denom = nh2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 1e-7);
}

float geometry_schlick_ggx(float NdotX, float roughness)
{
    float a = roughness * roughness;                 // α
    float k = (a + 1.0) * (a + 1.0) * 0.125;        // (α+1)^2 / 8  (direct lighting)
    float nx = max(NdotX, 1e-4);
    return nx / (nx * (1.0 - k) + k);
}

float geometry_smith(float NdotV, float NdotL, float roughness)
{
    return geometry_schlick_ggx(NdotV, roughness) *
           geometry_schlick_ggx(NdotL, roughness);
}

float3 fresnel_schlick(float cosTheta, float3 F0)
{
    float c = saturate(cosTheta);
    return F0 + (1.0 - F0) * pow(1.0 - c, 5.0);
}

// Cook–Torrance BRDF (direct light)
float3 calculate_pbr(
    float3 V,          // from surface to camera (normalized)
    float3 N,          // world normal (normalized)
    float3 L,          // from surface to light (normalized)
    float3 albedo,     // linear baseColor
    float  ao,
    float  roughness,  // perceptual [0,1]
    float  metallic,   // [0,1]
    float3 radiance    // lightColor * attenuation * intensity
)
{
    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0.0) return 0.0.xxx;

    float NdotV = saturate(dot(N, V));
    float3 H    = normalize(V + L);
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // Base reflectance
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Microfacet terms
    float  D  = distribution_ggx(NdotH, roughness);
    float  G  = geometry_smith(NdotV, NdotL, roughness);
    float3 F  = fresnel_schlick(VdotH, F0);

    // Specular
    float  denom = max(4.0 * NdotL * NdotV, 1e-4);
    float3 spec  = (D * G * F) / denom;

    // diffuse (Lambert) with energy conservation
    float3 kS = F;
    float3 kD = (1.0.xxx - kS) * (1.0 - metallic);
    float3 diff = (albedo / PI) * kD;

    // BRDF * NdotL * radiance
    float3 Lo = (diff + spec) * (NdotL * radiance);
    float3 Lo_diff = (albedo / PI) * kD * (NdotL * radiance) * ao;
    float3 Lo_spec = spec * (NdotL * radiance);

    return Lo_diff + Lo_spec;
}
