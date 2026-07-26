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

static const uint SFG_FOG_TYPE_LINEAR = 0;
static const uint SFG_FOG_TYPE_EXPONENTIAL = 1;
static const uint SFG_FOG_TYPE_EXPONENTIAL_SQUARED = 2;
static const uint SFG_FOG_TYPE_EXPONENTIAL_HEIGHT = 3;

float evaluate_fog_amount(float3 world_position, float3 camera_position, render_pass_data_fog fog_data)
{
    const float3 camera_to_surface = world_position - camera_position;
    const float surface_distance = length(camera_to_surface);
    const float start_distance = fog_data.distance_height.y;
    const float fog_distance = max(surface_distance - start_distance, 0.0);

    if (fog_data.color_intensity.w <= 0.0 || fog_distance <= 0.0)
        return 0.0;

    float fog_amount = 0.0;

    if (fog_data.type == SFG_FOG_TYPE_LINEAR)
    {
        fog_amount = saturate(fog_distance / max(fog_data.distance_height.z - start_distance, 0.0001));
    }
    else if (fog_data.type == SFG_FOG_TYPE_EXPONENTIAL)
    {
        fog_amount = 1.0 - exp(-fog_data.distance_height.x * fog_distance);
    }
    else if (fog_data.type == SFG_FOG_TYPE_EXPONENTIAL_SQUARED)
    {
        const float extinction = fog_data.distance_height.x * fog_distance;
        fog_amount = 1.0 - exp(-(extinction * extinction));
    }
    else
    {
        const float3 view_direction = camera_to_surface / max(surface_distance, 0.0001);
        const float3 fog_start_position = camera_position + view_direction * min(start_distance, surface_distance);
        const float height_scale = view_direction.y * fog_data.height_falloff;
        const float density_at_start = fog_data.distance_height.x * exp(clamp(-(fog_start_position.y - fog_data.distance_height.w) * fog_data.height_falloff, -80.0, 80.0));
        float optical_depth = density_at_start * fog_distance;

        if (abs(height_scale) > 0.0001)
            optical_depth = density_at_start * (1.0 - exp(clamp(-height_scale * fog_distance, -80.0, 80.0))) / height_scale;

        fog_amount = 1.0 - exp(-max(optical_depth, 0.0));
    }

    return min(saturate(fog_amount * fog_data.color_intensity.w), saturate(fog_data.max_opacity));
}

float3 apply_fog(float3 scene_color, float3 world_position, float3 camera_position, render_pass_data_fog fog_data)
{
    return lerp(scene_color, fog_data.color_intensity.rgb, evaluate_fog_amount(world_position, camera_position, fog_data));
}
