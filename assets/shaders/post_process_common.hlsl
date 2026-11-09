/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#ifndef SFG_POST_PROCESS_COMMON_HLSL
#define SFG_POST_PROCESS_COMMON_HLSL

struct render_pass_data
{
	float4 params0;
	float4 params1;
};

float3 white_balance(float3 color, float temperature, float tint)
{
	float3 gains = float3(1.0 + temperature * 0.1 - tint * 0.05, 1.0 + tint * 0.1, 1.0 - temperature * 0.1 - tint * 0.05);
	return max(color * gains, 0.0);
}

float3 adjust_saturation(float3 color, float saturation)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	return lerp(luminance.xxx, color, saturation);
}

// Stephen Hill, RRTAndODTFit curve - https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
float3 aces_fitted(float3 color)
{
	float3 a = color * (color + 0.0245786) - 0.000090537;
	float3 b = color * (0.983729 * color + 0.432951) + 0.238081;
	return saturate(a / b);
}

// Erik Reinhard, Michael Stark, Peter Shirley, James Ferwerda - https://www-old.cs.utah.edu/docs/techreports/2002/pdf/UUCS-02-001.pdf
float3 reinhard_extended(float3 color, float white_point)
{
	float white_squared = white_point * white_point;
	return saturate(color * (1.0 + color / white_squared) / (1.0 + color));
}

float3 apply_post_process(float3 color, render_pass_data rp_data)
{
	color *= exp2(rp_data.params0.y);
	color = white_balance(max(color, 0.0), rp_data.params1.x, rp_data.params1.y);
	color = adjust_saturation(color, rp_data.params0.z);
	uint tonemap_mode = (uint)rp_data.params1.z;
	if (tonemap_mode == 0)
		color = aces_fitted(color);
	else if (tonemap_mode == 1)
		color = reinhard_extended(color, max(rp_data.params0.w, 0.001));
	else
		color = saturate(color);

	return color;
}

#endif
