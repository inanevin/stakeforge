#include "layout_defines.hlsl"

struct vs_input
{
	float3 position : POSITION0;
	float3 other_position : POSITION1;
	float4 color : COLOR0;
	float corner : TEXCOORD0;
	float signed_thickness_px : TEXCOORD1;
};

struct vs_output
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	noperspective float2 line_coordinate : TEXCOORD0;
	nointerpolation float2 line_params : TEXCOORD1;
	nointerpolation float depth_mode : TEXCOORD2;
	float clip_distance : SV_ClipDistance0;
};

struct debug_line_data
{
	float4x4 view;
	float4x4 proj;
	float4 params;
};

vs_output VSMain(vs_input input)
{
	vs_output output = (vs_output)0;
	debug_line_data data = sfg_get_cbv<debug_line_data>(sfg_constant_rp0);
	float4 current_view = mul(data.view, float4(input.position, 1.0));
	float4 other_view = mul(data.view, float4(input.other_position, 1.0));
	float endpoint = input.corner >= 2.0 ? 1.0 : -1.0;
	float side = fmod(input.corner, 2.0) < 0.5 ? -1.0 : 1.0;
	float near_z = -data.params.z;
	bool current_clipped = current_view.z > near_z;
	bool other_clipped = other_view.z > near_z;
	if (current_clipped && other_clipped)
	{
		output.pos = float4(0.0, 0.0, 0.0, 1.0);
		output.clip_distance = -1.0;
		return output;
	}
	if (current_clipped)
	{
		float amount = (near_z - current_view.z) / (other_view.z - current_view.z);
		current_view = lerp(current_view, other_view, amount);
	}
	if (other_clipped)
	{
		float amount = (near_z - other_view.z) / (current_view.z - other_view.z);
		other_view = lerp(other_view, current_view, amount);
	}

	float4 current_clip = mul(data.proj, current_view);
	float4 other_clip = mul(data.proj, other_view);
	float2 viewport = data.params.xy;
	float2 current_ndc = current_clip.xy / current_clip.w;
	float2 other_ndc = other_clip.xy / other_clip.w;
	float2 current_screen = current_ndc * float2(viewport.x, -viewport.y) * 0.5;
	float2 other_screen = other_ndc * float2(viewport.x, -viewport.y) * 0.5;
	float2 screen_delta = other_screen - current_screen;
	float segment_length = length(screen_delta);
	float2 direction_to_other = segment_length > 0.001 ? screen_delta / segment_length : float2(1.0, 0.0);
	float2 direction = direction_to_other * -endpoint;
	float2 normal = float2(-direction.y, direction.x);
	float half_width = max(abs(input.signed_thickness_px) * 0.5, 0.5);
	float extent = half_width + 1.0;
	float2 offset_pixels = direction * endpoint * extent + normal * side * extent;
	float2 offset_ndc = offset_pixels * float2(2.0 / viewport.x, -2.0 / viewport.y);

	output.pos = current_clip;
	output.pos.xy += offset_ndc * current_clip.w;
	output.color = input.color;
	output.line_coordinate = float2(endpoint < 0.0 ? -extent : segment_length + extent, side * extent);
	output.line_params = float2(segment_length, half_width);
	output.depth_mode = input.signed_thickness_px < 0.0 ? 1.0 : 0.0;
	output.clip_distance = 1.0;
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	float closest_x = clamp(input.line_coordinate.x, 0.0, input.line_params.x);
	float distance_to_line = length(input.line_coordinate - float2(closest_x, 0.0));
	float coverage = 1.0 - smoothstep(input.line_params.y - 0.75, input.line_params.y + 0.75, distance_to_line);
	clip(coverage - 0.001);
	if (input.depth_mode < 0.5)
	{
		debug_line_data data = sfg_get_cbv<debug_line_data>(sfg_constant_rp0);
		Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(sfg_constant_obj0);
		float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));
		if (scene_depth > 0.000001 && input.pos.z + data.params.w < scene_depth)
			discard;
	}
	return float4(input.color.rgb, input.color.a * coverage);
}
