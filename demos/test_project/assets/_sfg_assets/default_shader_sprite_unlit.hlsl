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
