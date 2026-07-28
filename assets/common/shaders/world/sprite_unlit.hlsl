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

#include "sprite_common.hlsl"

SFG_MATERIAL_PARAM_VEC4("tint", sfg_color_hdr)

#ifdef USE_SELECTION

float4 PSMain(vs_output IN) : SV_TARGET
{
    sample_sprite(IN);
    return float4(1.0, 1.0, 1.0, 1.0);
}

#elif defined(WRITE_ID)

uint PSMain(vs_output IN) : SV_TARGET
{
    sample_sprite(IN);
    return IN.entity_id;
}

#elif defined(USE_ZPREPASS)

void PSMain(vs_output IN)
{
    sample_sprite(IN);
}

#elif defined(USE_GBUFFER)

struct ps_output
{
    float4 rt0 : SV_Target0;
    float4 rt1 : SV_Target1;
    float4 rt2 : SV_Target2;
    float4 rt3 : SV_Target3;
};

ps_output PSMain(vs_output IN)
{
    float4 color = sample_sprite(IN);

    ps_output OUT;
    OUT.rt0 = float4(color.rgb, 1.0);
    OUT.rt1 = float4(oct_encode(IN.world_normal), 0.0, 0.0);
    OUT.rt2 = float4(1.0, 1.0, 0.0, 1.0);
    OUT.rt3 = float4(color.rgb, 1.0);
    return OUT;
}

#else

float4 PSMain(vs_output IN) : SV_TARGET
{
    render_pass_data_view view_data = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    render_pass_data_fog fog_data = sfg_get_cbv<render_pass_data_fog>(SFG_RENDER_PASS_FOG);
    float4 color = sample_sprite(IN);

    if ((view_data.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG) != 0)
        color.rgb = apply_fog(color.rgb, IN.world_pos, view_data.camera_pos.xyz, fog_data);

    return color;
}

#endif
