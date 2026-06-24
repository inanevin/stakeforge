// Copyright (c) 2025 Inan Evin

#include "vekt_renderer.hpp"

#include "vekt/vg/vg_canvas.hpp"
#include "vekt/vg/vg_font.hpp"
#include "vekt/vg/vg_atlas.hpp"

#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/shader_description.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "math/math.hpp"
#include "math/vec2u16.hpp"

#include <cstring>

namespace sfg
{
	namespace
	{
		const char* k_shader_default = R"HLSL(
cbuffer projection_cb : register(b0, space0) { float4x4 projection; };

struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct VSOutput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	OUT.pos   = mul(projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET { return IN.color; }
)HLSL";

		const char* k_shader_text_bitmap = R"HLSL(
cbuffer projection_cb : register(b0, space0) { float4x4 projection; };
cbuffer mat_cb : register(b1, space0) { uint atlas_idx; uint _pad0; uint _pad1; uint _pad2; };
SamplerState samp_linear : register(s0, space0);

struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct VSOutput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	OUT.pos   = mul(projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas = ResourceDescriptorHeap[atlas_idx];
	float coverage  = atlas.SampleLevel(samp_linear, IN.uv, 0).r;
	return float4(IN.color.rgb, IN.color.a * coverage);
}
)HLSL";

		const char* k_shader_text_sdf = R"HLSL(
cbuffer projection_cb : register(b0, space0) { float4x4 projection; };
cbuffer mat_cb : register(b1, space0) { uint atlas_idx; float sdf_threshold; float sdf_softness; uint _pad0; };
SamplerState samp_linear : register(s0, space0);

struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct VSOutput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

VSOutput VSMain(VSInput IN)
{
	VSOutput OUT;
	OUT.pos   = mul(projection, float4(IN.pos, 0.0f, 1.0f));
	OUT.uv    = IN.uv;
	OUT.color = IN.color;
	return OUT;
}

float4 PSMain(VSOutput IN) : SV_TARGET
{
	Texture2D atlas = ResourceDescriptorHeap[atlas_idx];
	float d         = atlas.SampleLevel(samp_linear, IN.uv, 0).r;
	float w         = max(sdf_softness, fwidth(d));
	float coverage  = smoothstep(sdf_threshold - w, sdf_threshold + w, d);
	return float4(IN.color.rgb, IN.color.a * coverage);
}
)HLSL";

		void make_ortho(f32 out[16], f32 left, f32 right, f32 top, f32 bottom)
		{
			const f32 rl = right - left;
			const f32 bt = bottom - top;
			std::memset(out, 0, sizeof(f32) * 16);
			out[0]	= 2.0f / rl;
			out[5]	= 2.0f / -bt;
			out[10] = 1.0f;
			out[12] = -(right + left) / rl;
			out[13] = -(top + bottom) / -bt;
			out[15] = 1.0f;
		}

		bool compile_pair(gfx_backend* backend, const char* src, span_t<u8>& vs_out, span_t<u8>& ps_out)
		{
			const string_t	   source(src);
			vector_t<string_t> defines;
			vector_t<string_t> include_paths;
			span_t<u8>		   dummy_layout = {};
			if (!backend->compile_shader_vertex_pixel(static_cast<u8>(shader_stage::vertex), source, defines, include_paths, "VSMain", vs_out, false, dummy_layout))
				return false;
			if (!backend->compile_shader_vertex_pixel(static_cast<u8>(shader_stage::fragment), source, defines, include_paths, "PSMain", ps_out, false, dummy_layout))
				return false;
			return true;
		}

		shader_desc_t make_shader_desc(const char* name, bool blend)
		{
			shader_desc_t d						   = {};
			d.vertex_entry						   = "VSMain";
			d.pixel_entry						   = "PSMain";
			d.topo								   = topology::triangle_list;
			d.cull								   = cull_mode::none;
			d.fill								   = fill_mode::solid;
			d.front								   = front_face::cw;
			d.poly_mode							   = polygon_mode::fill;
			d.samples							   = 1;
			d.debug_name						   = name;
			d.depth_stencil_desc.flags			   = 0;
			d.depth_stencil_desc.attachment_format = format_t::undefined;

			vertex_input_t pos_input = {.name = "POSITION", .location = 0, .index = 0, .offset = 0, .size = sizeof(f32) * 2, .format = format_t::r32g32_sfloat};
			vertex_input_t uv_input	 = {.name = "TEXCOORD", .location = 0, .index = 0, .offset = sizeof(f32) * 2, .size = sizeof(f32) * 2, .format = format_t::r32g32_sfloat};
			vertex_input_t col_input = {.name = "COLOR", .location = 0, .index = 0, .offset = sizeof(f32) * 4, .size = sizeof(f32) * 4, .format = format_t::r32g32b32a32_sfloat};
			d.inputs.push_back(pos_input);
			d.inputs.push_back(uv_input);
			d.inputs.push_back(col_input);

			shader_color_attachment_t att				= {};
			att.format									= format_t::b8g8r8a8_srgb;
			att.blend_attachment.blend_enabled			= blend;
			att.blend_attachment.src_color_blend_factor = blend_factor::src_alpha;
			att.blend_attachment.dst_color_blend_factor = blend_factor::one_minus_src_alpha;
			att.blend_attachment.color_blend_op			= blend_op::add;
			att.blend_attachment.src_alpha_blend_factor = blend_factor::one;
			att.blend_attachment.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha;
			att.blend_attachment.alpha_blend_op			= blend_op::add;
			d.attachments.push_back(att);

			return d;
		}
	}

	bool vekt_renderer_t::init()
	{
		gfx_backend* backend = gfx_backend::get();
		SFG_ASSERT(backend != nullptr);

		_layout = backend->create_empty_bind_layout();
		backend->bind_layout_add_constant(_layout, 16, 0, 0, shader_stage::vertex);
		backend->bind_layout_add_constant(_layout, 4, 0, 1, shader_stage::fragment);

		sampler_desc_t samp = {};
		samp.flags			= sampler_flags::saf_min_linear | sampler_flags::saf_mag_linear | sampler_flags::saf_mip_linear | sampler_flags::saf_border_transparent;
		samp.address_u		= address_mode::clamp;
		samp.address_v		= address_mode::clamp;
		samp.address_w		= address_mode::clamp;
		samp.min_lod		= 0.0f;
		samp.max_lod		= 0.0f;
		backend->bind_layout_add_immutable_sampler(_layout, 0, 0, samp, shader_stage::fragment);

		backend->finalize_bind_layout(_layout, false, true, "vekt_layout");

		if (!create_pipelines())
			return false;

		for (u32 i = 0; i < 3; ++i)
		{
			pfd_t& p = _pfd[i];

			resource_desc_t v_desc = {};
			v_desc.size			   = vtx_capacity_bytes;
			v_desc.flags		   = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible;
			v_desc.debug_name	   = "vekt_vtx_upload";
			p.vertex_buffer		   = backend->create_resource(v_desc);
			backend->map_resource(p.vertex_buffer, p.mapped_vtx);

			resource_desc_t i_desc = {};
			i_desc.size			   = idx_capacity_bytes;
			i_desc.flags		   = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible;
			i_desc.debug_name	   = "vekt_idx_upload";
			p.index_buffer		   = backend->create_resource(i_desc);
			backend->map_resource(p.index_buffer, p.mapped_idx);

			resource_desc_t c_desc = {};
			c_desc.size			   = const_capacity_bytes;
			c_desc.flags		   = resource_flags::rf_cpu_visible;
			c_desc.debug_name	   = "vekt_const_upload";
			p.constants_buffer	   = backend->create_resource(c_desc);
			backend->map_resource(p.constants_buffer, p.mapped_const);
		}

		_atlas_lookup.reserve(16);

		return true;
	}

	bool vekt_renderer_t::create_pipelines()
	{
		gfx_backend* backend = gfx_backend::get();

		auto make_pipe = [&](const char* src, const char* name, gfx_handle& out_handle) -> bool {
			span_t<u8> vs_blob = {};
			span_t<u8> ps_blob = {};
			if (!compile_pair(backend, src, vs_blob, ps_blob))
				return false;
			vector_t<shader_blob_t> blobs;
			blobs.push_back({.stage = shader_stage::vertex, .data = vs_blob});
			blobs.push_back({.stage = shader_stage::fragment, .data = ps_blob});
			shader_desc_t desc = make_shader_desc(name, true);
			out_handle		   = backend->create_shader(desc, blobs, _layout, {});
			delete[] vs_blob.data;
			delete[] ps_blob.data;
			return !out_handle.is_null();
		};

		if (!make_pipe(k_shader_default, "vekt_default", _shader_default))
			return false;
		if (!make_pipe(k_shader_text_bitmap, "vekt_text", _shader_text_bitmap))
			return false;
		if (!make_pipe(k_shader_text_sdf, "vekt_sdf", _shader_text_sdf))
			return false;

		return true;
	}

	void vekt_renderer_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (auto& kv : _atlases)
		{
			if (!kv.second.texture.is_null())
				backend->destroy_texture(kv.second.texture);
			if (!kv.second.staging.is_null())
				backend->destroy_resource(kv.second.staging);
		}
		_atlases.clear();

		for (u32 i = 0; i < 3; ++i)
		{
			pfd_t& p = _pfd[i];
			if (!p.vertex_buffer.is_null())
				backend->destroy_resource(p.vertex_buffer);
			if (!p.index_buffer.is_null())
				backend->destroy_resource(p.index_buffer);
			if (!p.constants_buffer.is_null())
				backend->destroy_resource(p.constants_buffer);
			p = {};
		}

		if (!_shader_default.is_null())
			backend->destroy_shader(_shader_default);
		if (!_shader_text_bitmap.is_null())
			backend->destroy_shader(_shader_text_bitmap);
		if (!_shader_text_sdf.is_null())
			backend->destroy_shader(_shader_text_sdf);
		if (!_layout.is_null())
			backend->destroy_bind_layout(_layout);

		_shader_default		= {};
		_shader_text_bitmap = {};
		_shader_text_sdf	= {};
		_layout				= {};
		_atlas_lookup.resize(0);
	}

	void vekt_renderer_t::begin_frame(u8 frame_index)
	{
		_frame_index					= frame_index % 3;
		_pfd[_frame_index].vtx_offset	= 0;
		_pfd[_frame_index].idx_offset	= 0;
		_pfd[_frame_index].const_offset = 0;
	}

	vekt_renderer_t::atlas_gpu_t& vekt_renderer_t::sync_one_atlas(gfx_handle cmd, vekt::vg_atlas_t* atlas)
	{
		gfx_backend* backend = gfx_backend::get();
		auto&		 entry	 = _atlases[atlas];

		if (entry.texture.is_null())
		{
			texture_desc_t desc = {};
			desc.texture_format = format_t::r8_unorm;
			desc.size			= {static_cast<u16>(atlas->get_width()), static_cast<u16>(atlas->get_height())};
			desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
			desc.mip_levels		= 1;
			desc.array_length	= 1;
			desc.samples		= 1;
			desc.debug_name		= "vekt_atlas";
			entry.texture		= backend->create_texture(desc);
			entry.width			= atlas->get_width();
			entry.height		= atlas->get_height();
			entry.gpu_index		= backend->get_texture_gpu_index(entry.texture, 0);

			resource_desc_t s_desc = {};
			s_desc.size			   = entry.width * entry.height * 4;
			s_desc.flags		   = resource_flags::rf_cpu_visible;
			s_desc.debug_name	   = "vekt_atlas_staging";
			entry.staging		   = backend->create_resource(s_desc);
		}

		if (atlas->is_dirty())
		{
			texture_buffer_t tb = {};
			tb.pixels			= atlas->get_data();
			tb.bpp				= atlas->get_is_lcd() ? 3u : 1u;
			tb.size				= {static_cast<u16>(entry.width), static_cast<u16>(entry.height)};

			barrier_t pre	= {};
			pre.from_states = entry.transitioned ? resource_state_ps_resource : resource_state_common;
			pre.to_states	= resource_state_copy_dest;
			pre.texture_t	= entry.texture;
			pre.flags		= barrier_flags::baf_is_texture;
			backend->cmd_barrier(cmd, {.barriers = &pre, .barrier_count = 1});

			command_copy_buffer_to_texture_t cmdc = {};
			cmdc.textures						  = &tb;
			cmdc.destination_texture			  = entry.texture;
			cmdc.intermediate_buffer			  = entry.staging;
			cmdc.mip_levels						  = 1;
			cmdc.destination_slice				  = 0;
			backend->cmd_copy_buffer_to_texture(cmd, cmdc);

			barrier_t post	 = {};
			post.from_states = resource_state_copy_dest;
			post.to_states	 = resource_state_ps_resource;
			post.texture_t	 = entry.texture;
			post.flags		 = barrier_flags::baf_is_texture;
			backend->cmd_barrier(cmd, {.barriers = &post, .barrier_count = 1});

			entry.transitioned = true;
			atlas->clear_dirty();
		}

		return entry;
	}

	void vekt_renderer_t::sync_atlases(gfx_handle cmd, vekt::vg_font_manager_t& fonts)
	{
		for (vekt::vg_atlas_t* atlas : fonts.atlases())
			sync_one_atlas(cmd, atlas);
	}

	void vekt_renderer_t::draw(gfx_handle cmd, vekt::vg_canvas_t& canvas, vekt::vg_font_manager_t& fonts, const vec2u16_t& fb_size)
	{
		gfx_backend* backend = gfx_backend::get();
		pfd_t&		 p		 = _pfd[_frame_index];

		const auto& draw_buffers = canvas.draw_buffers();
		if (draw_buffers.empty())
			return;

		_atlas_lookup.resize(0);
		for (vekt::vg_atlas_t* atlas : fonts.atlases())
		{
			auto it = _atlases.find(atlas);
			if (it != _atlases.end())
				_atlas_lookup.push_back({atlas->get_id(), it->second.gpu_index});
		}

		f32 projection[16];
		make_ortho(projection, 0.0f, static_cast<f32>(fb_size.x), 0.0f, static_cast<f32>(fb_size.y));

		command_bind_layout_t bl = {.layout = _layout};
		backend->cmd_bind_layout(cmd, bl);

		command_bind_constants_t bc_proj = {.data = projection, .offset = 0, .count = 16, .param_index = 0};
		backend->cmd_bind_constants(cmd, bc_proj);

		command_set_viewport_t vp = {.x = 0.0f, .y = 0.0f, .min_depth = 0.0f, .max_depth = 1.0f, .width = fb_size.x, .height = fb_size.y};
		backend->cmd_set_viewport(cmd, vp);

		gfx_handle current_pipeline = {};

		for (const vekt::vg_draw_buffer_t& db : draw_buffers)
		{
			if (db.vertex_count == 0 || db.index_count == 0)
				continue;

			const u32 vtx_size = db.vertex_count * sizeof(vekt::vg_vertex_t);
			const u32 idx_size = db.index_count * sizeof(vekt::vg_index_t);

			if (p.vtx_offset + vtx_size > vtx_capacity_bytes || p.idx_offset + idx_size > idx_capacity_bytes)
			{
				SFG_ERR("vekt_renderer: per-frame upload buffer exhausted, dropping draw");
				continue;
			}

			std::memcpy(p.mapped_vtx + p.vtx_offset, db.vertex_start, vtx_size);
			std::memcpy(p.mapped_idx + p.idx_offset, db.index_start, idx_size);

			gfx_handle pipe;
			if (db.font_id == 0xFFFFFFFFu)
				pipe = _shader_default;
			else if (db.font_kind == vekt::vg_font_kind_e::sdf)
				pipe = _shader_text_sdf;
			else
				pipe = _shader_text_bitmap;

			if (pipe != current_pipeline)
			{
				command_bind_pipeline_t bp = {.pipeline = pipe};
				backend->cmd_bind_pipeline(cmd, bp);
				current_pipeline = pipe;
			}

			struct mat_const_t
			{
				u32 atlas_idx	  = 0;
				f32 sdf_threshold = 0.5f;
				f32 sdf_softness  = 0.0625f;
				u32 _pad		  = 0;
			} mc;

			if (db.atlas_id != 0xFFFFFFFFu)
			{
				for (const atlas_lookup_entry_t& e : _atlas_lookup)
				{
					if (e.atlas_id == db.atlas_id)
					{
						mc.atlas_idx = e.gpu_index;
						break;
					}
				}
			}

			command_bind_constants_t bc_mat = {.data = &mc, .offset = 0, .count = 4, .param_index = 1};
			backend->cmd_bind_constants(cmd, bc_mat);

			const u16			   sx = static_cast<u16>(math::max(0.0f, db.clip.x));
			const u16			   sy = static_cast<u16>(math::max(0.0f, db.clip.y));
			const u16			   sw = static_cast<u16>(math::max(0.0f, db.clip.z));
			const u16			   sh = static_cast<u16>(math::max(0.0f, db.clip.w));
			command_set_scissors_t sc = {.x = sx, .y = sy, .width = sw, .height = sh};
			backend->cmd_set_scissors(cmd, sc);

			command_bind_vertex_buffers_t vb = {};
			vb.buffer_t						 = p.vertex_buffer;
			vb.slot							 = 0;
			vb.vertex_size					 = sizeof(vekt::vg_vertex_t);
			vb.offset						 = p.vtx_offset;
			backend->cmd_bind_vertex_buffers(cmd, vb);

			command_bind_index_buffers_t ib = {};
			ib.buffer_t						= p.index_buffer;
			ib.offset						= p.idx_offset;
			ib.index_size					= sizeof(vekt::vg_index_t);
			backend->cmd_bind_index_buffers(cmd, ib);

			command_draw_indexed_instanced_t draw_cmd = {};
			draw_cmd.index_count_per_instance		  = db.index_count;
			draw_cmd.instance_count					  = 1;
			draw_cmd.start_index_location			  = 0;
			draw_cmd.base_vertex_location			  = 0;
			draw_cmd.start_instance_location		  = 0;
			backend->cmd_draw_indexed_instanced(cmd, draw_cmd);

			p.vtx_offset += vtx_size;
			p.idx_offset += idx_size;
		}
	}
}
