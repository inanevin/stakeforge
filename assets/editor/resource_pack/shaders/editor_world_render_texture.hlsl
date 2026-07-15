#include "layout_defines.hlsl"

struct vs_output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

SamplerState smp_linear : static_sampler_linear;

struct editor_world_render_data
{
	float4x4 proj;
	float4x4 inv_proj;
	float4x4 inv_view;
	float4 camera_position;
	float4 camera_grid;
	float4 grid_params;
	float4 grid_minor_color;
	float4 grid_major_color;
	float4 grid_x_color;
	float4 grid_z_color;
	float4 params;
	float4 selection_color;
};

float grid_line(float2 position, float spacing)
{
	float2 coordinate = position / spacing;
	float2 width = max(fwidth(coordinate), 0.00001);
	float2 distance = abs(frac(coordinate - 0.5) - 0.5);
	float2 line_value = saturate(1.0 - distance / width);
	return max(line_value.x, line_value.y);
}

vs_output VSMain(uint vertex_id : SV_VertexID)
{
	vs_output output;
	float2	  pos;
	if (vertex_id == 0)
		pos = float2(-1.0, -1.0);
	else if (vertex_id == 1)
		pos = float2(-1.0, 3.0);
	else
		pos = float2(3.0, -1.0);

	output.pos = float4(pos, 0.0, 1.0);
	output.uv	= pos * float2(0.5, -0.5) + 0.5;
	return output;
}

float4 PSMain(vs_output input) : SV_TARGET
{
	editor_world_render_data data = sfg_get_cbv<editor_world_render_data>(sfg_constant_rp0);
	Texture2D source = sfg_get_texture<Texture2D>(sfg_constant_obj0);
	float4 color = source.SampleLevel(smp_linear, input.uv, 0);
	if (data.camera_grid.w > 0.0)
	{
		float2 ndc = float2(input.uv.x * 2.0 - 1.0, 1.0 - input.uv.y * 2.0);
		float4 far_view = mul(data.inv_proj, float4(ndc, 0.0, 1.0));
		float3 view_ray = normalize(far_view.xyz / far_view.w);
		float3 world_ray = normalize(mul((float3x3)data.inv_view, view_ray));
		float ray_distance = -data.camera_position.y / world_ray.y;
		if (abs(world_ray.y) > 0.0001 && ray_distance > 0.0)
		{
			float4 plane_clip = mul(data.proj, float4(view_ray * ray_distance, 1.0));
			float plane_depth = plane_clip.z / plane_clip.w;
			Texture2D<float> depth_texture = sfg_get_texture<Texture2D<float> >(sfg_constant_obj2);
			float scene_depth = depth_texture.Load(int3(uint2(input.pos.xy), 0));
			if (scene_depth <= 0.000001 || plane_depth > scene_depth + 0.000001)
			{
				float grid_scale = data.grid_params.x;
				float2 relative_position = world_ray.xz * ray_distance;
				float2 grid_position = data.camera_grid.xz + relative_position;
				float footprint = max(length(ddx(grid_position)), length(ddy(grid_position)));
				float lod_value = clamp(log10(max(footprint * 8.0 / grid_scale, 1.0)), 0.0, 2.0);
				float lod_floor = floor(lod_value);
				float lod_blend = frac(lod_value);
				float spacing = grid_scale * pow(10.0, lod_floor);
				float fine_line = grid_line(grid_position, spacing);
				float medium_line = grid_line(grid_position, spacing * 10.0);
				float coarse_line = grid_line(grid_position, spacing * 100.0);
				float fine_only = fine_line * (1.0 - medium_line);
				float medium_only = medium_line * (1.0 - coarse_line);
				float fade_start = min(max(grid_scale * 25.0, data.grid_params.z * 0.05), data.grid_params.z * 0.5);
				float fade_end = min(max(grid_scale * 100.0, data.grid_params.z * 0.35), data.grid_params.z * 0.9);
				float fade = (1.0 - smoothstep(fade_start, fade_end, ray_distance)) * smoothstep(0.015, 0.12, abs(world_ray.y));
				float medium_alpha = lerp(data.grid_major_color.a, data.grid_minor_color.a, lod_blend);
				float3 medium_color = lerp(data.grid_major_color.rgb, data.grid_minor_color.rgb, lod_blend);
				color.rgb = lerp(color.rgb, data.grid_minor_color.rgb, fine_only * data.grid_minor_color.a * (1.0 - lod_blend) * fade);
				color.rgb = lerp(color.rgb, medium_color, medium_only * medium_alpha * fade);
				color.rgb = lerp(color.rgb, data.grid_major_color.rgb, coarse_line * data.grid_major_color.a * fade);

				float2 absolute_position = data.camera_position.xz + relative_position;
				float x_axis = saturate(1.0 - abs(absolute_position.y) / max(fwidth(absolute_position.y) * 1.5, 0.00001));
				float z_axis = saturate(1.0 - abs(absolute_position.x) / max(fwidth(absolute_position.x) * 1.5, 0.00001));
				color.rgb = lerp(color.rgb, data.grid_x_color.rgb, x_axis * data.grid_x_color.a * fade);
				color.rgb = lerp(color.rgb, data.grid_z_color.rgb, z_axis * data.grid_z_color.a * fade);
			}
		}
	}
	if (data.params.w <= 0.0)
		return color;

	Texture2D selection = sfg_get_texture<Texture2D>(sfg_constant_obj1);
	float2 texel = 1.0 / data.params.xy;
	float thickness = data.params.z;
	float2 diff_uv = texel * thickness;
	float4 x_diff = selection.SampleLevel(smp_linear, input.uv - float2(diff_uv.x, 0.0), 0);
	x_diff -= selection.SampleLevel(smp_linear, input.uv + float2(diff_uv.x, 0.0), 0);
	float4 y_diff = selection.SampleLevel(smp_linear, input.uv - float2(0.0, diff_uv.y), 0);
	y_diff -= selection.SampleLevel(smp_linear, input.uv + float2(0.0, diff_uv.y), 0);
	float outline = saturate(length(sqrt(x_diff * x_diff + y_diff * y_diff).rgb));
	return float4(saturate(color.rgb + data.selection_color.rgb * outline), color.a);
}
