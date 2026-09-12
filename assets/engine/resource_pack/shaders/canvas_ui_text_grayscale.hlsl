// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//  This file is a part of: Stakeforge Engine
//  https://github.com/inanevin/StakeforgeEngine
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

#include "../../../common/shaders/layout_defines.hlsl"

struct projection_cb
{
	float4x4 projection;
};

SamplerState samp_nearest : static_sampler_nearest;

struct VSInput
{
	float2 pos   : POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

struct VSOutput
{
	float4 pos   : SV_POSITION;
	float2 uv    : TEXCOORD0;
	float4 color : COLOR0;
};

VSOutput VSMain(VSInput IN)
{
	ConstantBuffer<projection_cb> proj = sfg_get_cbv<projection_cb>(sfg_constant_rp0);
	VSOutput OUT;
	OUT.pos   = mul(proj.projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas	  = sfg_get_texture<Texture2D>(sfg_constant_mat0);
	float	  coverage = atlas.SampleLevel(samp_nearest, IN.uv, 0).r;
	return float4(IN.color.rgb, IN.color.a * coverage);
}
