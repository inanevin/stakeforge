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

#include "layout_defines.hlsl"
#include "render_pass_defines.hlsl"
#include "normal.hlsl"
#include "fog.hlsl"

SFG_MATERIAL_TEXTURE("line", sfg_texture2d)
SFG_MATERIAL_SAMPLER("line")
SFG_MATERIAL_PARAM_VEC4("tint", sfg_color_hdr)

#define LINE_MITER_LIMIT 2.0

struct line_data
{
    float4 points[16];
    float4 start_color;
    float4 end_color;
    float start_width;
    float end_width;
    float texture_tiling;
    float total_length;
    uint point_count;
    uint closed;
    uint entity_id;
    uint entity_index;
};

struct material_data
{
    float4 tint;
};

struct vs_output
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 world_pos : TEXCOORD1;
    float4 color : COLOR0;
    nointerpolation uint entity_id : TEXCOORD2;
};

float2 segment_side(float3 from, float3 to, float4x4 view)
{
    float2 direction = mul(view, float4(to - from, 0.0)).xy;
    float length_squared = dot(direction, direction);
    return length_squared > 0.00000001 ? float2(-direction.y, direction.x) * rsqrt(length_squared) : float2(1.0, 0.0);
}

float2 joint_offset(float2 incoming, float2 outgoing, float2 segment, float side, float half_width, bool endpoint)
{
    if (endpoint)
        return segment * side * half_width;

    float2 sum = incoming + outgoing;
    float denominator = dot(sum, outgoing);
    float2 miter = denominator > 0.000001 ? sum / denominator : float2(0.0, 0.0);
    if (denominator > 0.000001 && dot(miter, miter) <= LINE_MITER_LIMIT * LINE_MITER_LIMIT)
        return miter * side * half_width;

    float turn = incoming.x * outgoing.y - incoming.y * outgoing.x;
    float outer_side = turn >= 0.0 ? -1.0 : 1.0;
    return side == outer_side ? segment * side * half_width : float2(0.0, 0.0);
}

vs_output VSMain(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    StructuredBuffer<line_data> lines = sfg_get_ssbo<line_data>(sfg_constant_obj0);
    StructuredBuffer<uint> instances = sfg_get_ssbo<uint>(sfg_constant_obj2);
    line_data path = lines[instances[sfg_constant_obj1 + instance_id]];
    render_pass_data_view view = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    uint segment = vertex_id / 12;
    uint corner = vertex_id % 12;
    uint next = (segment + 1) % path.point_count;
    uint previous = segment == 0 ? path.point_count - 1 : segment - 1;
    uint following = (next + 1) % path.point_count;
    bool start_cap = path.closed == 0 && segment == 0;
    bool end_cap = path.closed == 0 && next == path.point_count - 1;
    float2 side = segment_side(path.points[segment].xyz, path.points[next].xyz, view.view);
    float2 incoming = start_cap ? side : segment_side(path.points[previous].xyz, path.points[segment].xyz, view.view);
    float2 outgoing = end_cap ? side : segment_side(path.points[next].xyz, path.points[following].xyz, view.view);
    float end_distance = next == 0 ? path.total_length : path.points[next].w;
    float start_fraction = path.points[segment].w / max(path.total_length, 0.000001);
    float end_fraction = end_distance / max(path.total_length, 0.000001);
    float start_width = lerp(path.start_width, path.end_width, start_fraction) * 0.5;
    float end_width = lerp(path.start_width, path.end_width, end_fraction) * 0.5;

    static const uint ends[6] = {0, 1, 1, 0, 1, 0};
    static const float sides[6] = {-1.0, -1.0, 1.0, -1.0, 1.0, 1.0};
    bool at_end = corner >= 6 || ends[min(corner, 5)] != 0;
    float sign = corner < 6 ? sides[corner] : (corner < 9 ? -1.0 : 1.0);
    float2 offset = float2(0.0, 0.0);
    if (corner < 6)
        offset = at_end ? joint_offset(side, outgoing, side, sign, end_width, end_cap) : joint_offset(incoming, side, side, sign, start_width, start_cap);
    else if (!end_cap)
    {
        float2 a = joint_offset(side, outgoing, side, sign, end_width, false);
        float2 b = joint_offset(side, outgoing, outgoing, sign, end_width, false);
        uint join_corner = (corner - 6) % 3;
        offset = join_corner == 0 ? a : (join_corner == 1 ? b : (a + b) * 0.5);
        if (dot(a - b, a - b) > 0.00000001 && join_corner == 2)
            offset = float2(0.0, 0.0);
    }

    float3 position = at_end ? path.points[next].xyz : path.points[segment].xyz;
    position += mul(view.inv_view, float4(offset, 0.0, 0.0)).xyz;
    vs_output OUT;
    OUT.pos = mul(view.view_proj, float4(position, 1.0));
    OUT.uv = float2((at_end ? end_distance : path.points[segment].w) * path.texture_tiling, sign * 0.5 + 0.5);
    if (corner >= 6 && (corner - 6) % 3 == 2)
        OUT.uv.y = 0.5;
    OUT.world_pos = position;
    OUT.color = lerp(path.start_color, path.end_color, at_end ? end_fraction : start_fraction);
    OUT.entity_id = path.entity_id;
    return OUT;
}

float4 sample_line(vs_output IN)
{
    material_data material = sfg_get_cbv<material_data>(sfg_constant_mat0);
    Texture2D texture = sfg_get_texture<Texture2D>(sfg_constant_mat1);
    SamplerState line_sampler = sfg_get_sampler_state(sfg_constant_mat2);
    float4 color = texture.Sample(line_sampler, IN.uv) * material.tint * IN.color;
    clip(color.a - 0.001);
#ifdef LINE_PREMULTIPLIED_ALPHA
    color.rgb *= color.a;
#endif
    return color;
}

#ifdef USE_SELECTION
float4 PSMain(vs_output IN) : SV_TARGET
{
    sample_line(IN);
    return float4(1.0, 1.0, 1.0, 1.0);
}
#elif defined(WRITE_ID)
uint PSMain(vs_output IN) : SV_TARGET
{
    sample_line(IN);
    return IN.entity_id;
}
#elif defined(USE_ZPREPASS)
void PSMain(vs_output IN)
{
    sample_line(IN);
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
    float4 color = sample_line(IN);
    render_pass_data_view view = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    float3 normal = normalize(mul(view.inv_view, float4(0.0, 0.0, 1.0, 0.0)).xyz);
    ps_output OUT;
    OUT.rt0 = float4(color.rgb, 1.0);
    OUT.rt1 = float4(oct_encode(normal), 0.0, 0.0);
    OUT.rt2 = float4(1.0, 1.0, 0.0, 1.0);
    OUT.rt3 = float4(color.rgb, 1.0);
    return OUT;
}
#else
float4 PSMain(vs_output IN) : SV_TARGET
{
    float4 color = sample_line(IN);
    render_pass_data_view view = sfg_get_cbv<render_pass_data_view>(SFG_RENDER_PASS_VIEW);
    render_pass_data_fog fog = sfg_get_cbv<render_pass_data_fog>(SFG_RENDER_PASS_FOG);
    if ((view.flags & SFG_RENDER_PASS_VIEW_FLAG_SAMPLE_FOG) != 0)
        color.rgb = apply_fog(color.rgb, IN.world_pos, view.camera_pos.xyz, fog);
    return color;
}
#endif
