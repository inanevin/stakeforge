/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "vg_canvas.hpp"
#include "vg_path.hpp"
#include "vg_atlas.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>

#include <algorithm>
#include <cstring>

namespace sfg::ui
{
	namespace
	{
		constexpr u64 fnv_offset = 14695981039346656037ull;
		constexpr u64 fnv_prime	 = 1099511628211ull;

		inline u64 hash_bytes(u64 h, const void* ptr, size_t n)
		{
			const u8* b = static_cast<const u8*>(ptr);
			for (size_t i = 0; i < n; ++i)
			{
				h ^= b[i];
				h *= fnv_prime;
			}
			return h;
		}

		u64 hash_text(const char* text, size_t len, const vg_text_paint_t& p)
		{
			u64 h = fnv_offset;
			h	  = hash_bytes(h, text, len);
			h	  = hash_bytes(h, &p.font, sizeof(p.font));
			h	  = hash_bytes(h, &p.color, sizeof(p.color));
			h	  = hash_bytes(h, &p.scale, sizeof(p.scale));
			h	  = hash_bytes(h, &p.spacing, sizeof(p.spacing));
			h	  = hash_bytes(h, &p.flip_uv, sizeof(p.flip_uv));
			return h;
		}

		inline vg_vertex_t* take_vertices(vg_draw_buffer_t* db, u32 count)
		{
			SFG_ASSERT(db->vertex_count + count <= db->vertex_capacity);
			vg_vertex_t* p = db->vertex_start + db->vertex_count;
			db->vertex_count += count;
			return p;
		}

		inline vg_index_t* take_indices(vg_draw_buffer_t* db, u32 count)
		{
			SFG_ASSERT(db->index_count + count <= db->index_capacity);
			vg_index_t* p = db->index_start + db->index_count;
			db->index_count += count;
			return p;
		}

		void emit_path_solid(vg_draw_buffer_t* db, const vector_t<vec2f_t>& path, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max)
		{
			const f32	 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
			const f32	 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;
			vg_vertex_t* v	   = take_vertices(db, static_cast<u32>(path.size()));
			for (size_t i = 0; i < path.size(); ++i)
			{
				v[i].pos   = path[i];
				v[i].color = color;
				v[i].uv.x  = (path[i].x - min.x) * inv_x;
				v[i].uv.y  = (path[i].y - min.y) * inv_y;
			}
		}

		void emit_path_grad(vg_draw_buffer_t* db, const vector_t<vec2f_t>& path, const vec4f_t& color_a, const vec4f_t& color_b, vg_gradient_e dir, const vec2f_t& min, const vec2f_t& max)
		{
			const f32 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
			const f32 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;

			vg_vertex_t*  v	   = take_vertices(db, static_cast<u32>(path.size()));
			const vec4f_t diff = color_b - color_a;

			for (size_t i = 0; i < path.size(); ++i)
			{
				const f32 ux = (path[i].x - min.x) * inv_x;
				const f32 uy = (path[i].y - min.y) * inv_y;
				const f32 t	 = (dir == vg_gradient_e::horizontal) ? ux : uy;
				v[i].pos	 = path[i];
				v[i].color.x = color_a.x + diff.x * t;
				v[i].color.y = color_a.y + diff.y * t;
				v[i].color.z = color_a.z + diff.z * t;
				v[i].color.w = color_a.w + diff.w * t;
				v[i].uv.x	 = ux;
				v[i].uv.y	 = uy;
			}
		}

		void emit_central_solid(vg_draw_buffer_t* db, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max)
		{
			vg_vertex_t* v = take_vertices(db, 1);
			v[0].pos	   = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
			v[0].uv		   = {0.5f, 0.5f};
			v[0].color	   = color;
		}

		void emit_central_grad(vg_draw_buffer_t* db, const vec4f_t& color_a, const vec4f_t& color_b, const vec2f_t& min, const vec2f_t& max)
		{
			vg_vertex_t* v = take_vertices(db, 1);
			v[0].pos	   = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
			v[0].uv		   = {0.5f, 0.5f};
			v[0].color	   = (color_a + color_b) * 0.5f;
		}

		void emit_path_alpha(vg_draw_buffer_t* db, const vector_t<vec2f_t>& path, u32 source_vtx_base, f32 alpha, const vec2f_t& min, const vec2f_t& max)
		{
			const f32 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
			const f32 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;

			vg_vertex_t* v = take_vertices(db, static_cast<u32>(path.size()));
			for (size_t i = 0; i < path.size(); ++i)
			{
				v[i].pos	 = path[i];
				v[i].color	 = db->vertex_start[source_vtx_base + i].color;
				v[i].color.w = alpha;
				v[i].uv.x	 = (path[i].x - min.x) * inv_x;
				v[i].uv.y	 = (path[i].y - min.y) * inv_y;
			}
		}

		void emit_quad_indices(vg_draw_buffer_t* db, u32 base)
		{
			vg_index_t* idx = take_indices(db, 6);
			idx[0]			= static_cast<vg_index_t>(base + 0);
			idx[1]			= static_cast<vg_index_t>(base + 1);
			idx[2]			= static_cast<vg_index_t>(base + 3);
			idx[3]			= static_cast<vg_index_t>(base + 1);
			idx[4]			= static_cast<vg_index_t>(base + 2);
			idx[5]			= static_cast<vg_index_t>(base + 3);
		}

		void emit_fan_indices(vg_draw_buffer_t* db, u32 ring_base, u32 center_idx, u32 ring_size)
		{
			vg_index_t* idx = take_indices(db, ring_size * 3);
			for (u32 i = 0; i < ring_size; ++i)
			{
				idx[i * 3 + 0] = static_cast<vg_index_t>(center_idx);
				idx[i * 3 + 1] = static_cast<vg_index_t>(ring_base + i);
				idx[i * 3 + 2] = static_cast<vg_index_t>(ring_base + ((i + 1) % ring_size));
			}
		}

		void emit_strip_indices(vg_draw_buffer_t* db, u32 outer_base, u32 inner_base, u32 ring_size)
		{
			vg_index_t* idx = take_indices(db, ring_size * 6);
			for (u32 i = 0; i < ring_size; ++i)
			{
				const u32 o0   = outer_base + i;
				const u32 o1   = outer_base + ((i + 1) % ring_size);
				const u32 in0  = inner_base + i;
				const u32 in1  = inner_base + ((i + 1) % ring_size);
				const u32 base = i * 6;
				idx[base + 0]  = static_cast<vg_index_t>(o0);
				idx[base + 1]  = static_cast<vg_index_t>(o1);
				idx[base + 2]  = static_cast<vg_index_t>(in0);
				idx[base + 3]  = static_cast<vg_index_t>(o1);
				idx[base + 4]  = static_cast<vg_index_t>(in1);
				idx[base + 5]  = static_cast<vg_index_t>(in0);
			}
		}
	}

	vg_canvas_t::~vg_canvas_t()
	{
		SFG_ASSERT(_vertex_pool == nullptr);
	}

	void vg_canvas_t::init(const vg_canvas_config_t& cfg)
	{
		SFG_ASSERT(cfg.vertex_buffer_bytes > 0 && cfg.index_buffer_bytes > 0 && cfg.buffer_count > 0);

		const u64 vtx_count = cfg.vertex_buffer_bytes / sizeof(vg_vertex_t);
		const u64 idx_count = cfg.index_buffer_bytes / sizeof(vg_index_t);

		_vertex_pool				= static_cast<vg_vertex_t*>(SFG_MALLOC(sizeof(vg_vertex_t) * vtx_count));
		_index_pool					= static_cast<vg_index_t*>(SFG_MALLOC(sizeof(vg_index_t) * idx_count));
		_vertex_capacity_per_buffer = static_cast<u32>(vtx_count / cfg.buffer_count);
		_index_capacity_per_buffer	= static_cast<u32>(idx_count / cfg.buffer_count);
		_buffer_count				= cfg.buffer_count;

		const u64 cache_vtx = cfg.text_cache_vertex_bytes / sizeof(vg_vertex_t);
		const u64 cache_idx = cfg.text_cache_index_bytes / sizeof(vg_index_t);

		_text_cache_vertex_buffer	= static_cast<vg_vertex_t*>(SFG_MALLOC(sizeof(vg_vertex_t) * cache_vtx));
		_text_cache_index_buffer	= static_cast<vg_index_t*>(SFG_MALLOC(sizeof(vg_index_t) * cache_idx));
		_text_cache_vertex_capacity = static_cast<u32>(cache_vtx);
		_text_cache_index_capacity	= static_cast<u32>(cache_idx);

		const size_t total_bytes = sizeof(vg_vertex_t) * vtx_count + sizeof(vg_index_t) * idx_count + sizeof(vg_vertex_t) * cache_vtx + sizeof(vg_index_t) * cache_idx;
		SFG_MEMTRACE_ALLOC(_vertex_pool, total_bytes);

		_draw_buffers.reserve(cfg.buffer_count);
		_clip_stack.reserve(cfg.clip_stack_capacity);
		_text_cache.reserve(256);
		_path0.reserve(512);
		_path1.reserve(512);
		_path2.reserve(512);
	}

	void vg_canvas_t::uninit()
	{
		SFG_MEMTRACE_DEALLOC(_vertex_pool);
		if (_vertex_pool)
			SFG_FREE(_vertex_pool);
		if (_index_pool)
			SFG_FREE(_index_pool);
		if (_text_cache_vertex_buffer)
			SFG_FREE(_text_cache_vertex_buffer);
		if (_text_cache_index_buffer)
			SFG_FREE(_text_cache_index_buffer);

		_vertex_pool			  = nullptr;
		_index_pool				  = nullptr;
		_text_cache_vertex_buffer = nullptr;
		_text_cache_index_buffer  = nullptr;

		_draw_buffers.resize(0);
		_clip_stack.resize(0);
		_text_cache.resize(0);
		_path0.resize(0);
		_path1.resize(0);
		_path2.resize(0);
	}

	void vg_canvas_t::frame_begin(const vec4f_t& screen_clip)
	{
		_draw_buffers.resize(0);
		_clip_stack.resize(0);
		_buffer_counter = 0;
		_clip_stack.push_back({screen_clip});
	}

	void vg_canvas_t::frame_end()
	{
		std::sort(_draw_buffers.begin(), _draw_buffers.end(), [](const vg_draw_buffer_t& a, const vg_draw_buffer_t& b) { return a.draw_order < b.draw_order; });
	}

	void vg_canvas_t::push_clip(const vec4f_t& rect)
	{
		const vec4f_t base = current_clip();
		_clip_stack.push_back({intersect_clip(base, rect)});
	}

	void vg_canvas_t::pop_clip()
	{
		SFG_ASSERT(!_clip_stack.empty());
		_clip_stack.pop_back();
	}

	vec4f_t vg_canvas_t::current_clip() const
	{
		SFG_ASSERT(!_clip_stack.empty());
		return _clip_stack.back().rect;
	}

	vec4f_t vg_canvas_t::intersect_clip(const vec4f_t& a, const vec4f_t& b) const
	{
		const f32 x = math::max(a.x, b.x);
		const f32 y = math::max(a.y, b.y);
		const f32 r = math::min(a.x + a.z, b.x + b.z);
		const f32 t = math::min(a.y + a.w, b.y + b.w);
		if (r < x || t < y)
			return {0, 0, 0, 0};
		return {x, y, r - x, t - y};
	}

	vg_draw_buffer_t* vg_canvas_t::get_draw_buffer(u32 draw_order, void* user_data, font_data_t* font)
	{
		const vec4f_t clip	 = current_clip();
		const u32	  fnt_id = font ? static_cast<u32>(reinterpret_cast<uintptr_t>(font)) : invalid_id_u32;
		const u32	  atl_id = invalid_id_u32;
		const auto	  knd	 = font ? font->kind : font_kind_e::bitmap;

		for (vg_draw_buffer_t& db : _draw_buffers)
		{
			if (db.draw_order != draw_order)
				continue;
			if (db.user_data != user_data)
				continue;
			if (db.font_id != fnt_id)
				continue;
			if (!db.clip.equals(clip, 0.5f))
				continue;
			return &db;
		}

		SFG_ASSERT(_buffer_counter < _buffer_count);

		vg_draw_buffer_t db;
		db.vertex_start	   = _vertex_pool + _buffer_counter * _vertex_capacity_per_buffer;
		db.index_start	   = _index_pool + _buffer_counter * _index_capacity_per_buffer;
		db.vertex_capacity = _vertex_capacity_per_buffer;
		db.index_capacity  = _index_capacity_per_buffer;
		db.clip			   = clip;
		db.draw_order	   = draw_order;
		db.user_data	   = user_data;
		db.font_id		   = fnt_id;
		db.atlas_id		   = atl_id;
		db.font_kind	   = knd;
		_buffer_counter++;
		_draw_buffers.push_back(db);
		return &_draw_buffers.back();
	}

	void vg_canvas_t::add_rect(const vec2f_t& min, const vec2f_t& max, const vg_rect_paint_t& paint, u32 draw_order, void* user_data)
	{
		vg_draw_buffer_t* db = get_draw_buffer(draw_order, user_data, nullptr);

		const bool round = paint.rounding > 0.0f;
		const bool out	 = paint.outline_thickness > 0.0f;
		const bool aa	 = paint.aa_thickness > 0.0f;
		const bool grad	 = paint.gradient != vg_gradient_e::none;

		if (!paint.filled && !out)
			return;

		if (round)
			vg_path_rounded_rect(_path0, min, max, paint.rounding, paint.rounding_segs);
		else
			vg_path_sharp_rect(_path0, min, max);

		if (out)
		{
			const f32 dir = paint.filled ? 1.0f : -1.0f;
			vg_path_expand(_path1, _path0, paint.outline_thickness * dir);
		}

		const u32 fill_vtx_base = db->vertex_count;
		if (paint.filled)
		{
			if (grad)
				emit_path_grad(db, _path0, paint.fill_color_a, paint.fill_color_b, paint.gradient, min, max);
			else
				emit_path_solid(db, _path0, paint.fill_color_a, min, max);

			if (round)
			{
				const u32 center_idx = db->vertex_count;
				if (grad)
					emit_central_grad(db, paint.fill_color_a, paint.fill_color_b, min, max);
				else
					emit_central_solid(db, paint.fill_color_a, min, max);
				emit_fan_indices(db, fill_vtx_base, center_idx, static_cast<u32>(_path0.size()));
			}
			else
			{
				emit_quad_indices(db, fill_vtx_base);
			}
		}

		u32 outline_outer_base = invalid_id_u32;
		if (out)
		{
			outline_outer_base = db->vertex_count;
			if (paint.filled)
			{
				emit_path_solid(db, _path1, paint.outline_color, min, max);
				const u32 outline_inner_base = db->vertex_count;
				emit_path_solid(db, _path0, paint.outline_color, min, max);
				emit_strip_indices(db, outline_outer_base, outline_inner_base, static_cast<u32>(_path0.size()));
			}
			else
			{
				emit_path_solid(db, _path0, paint.outline_color, min, max);
				const u32 outline_inner_base = db->vertex_count;
				emit_path_solid(db, _path1, paint.outline_color, min, max);
				emit_strip_indices(db, outline_outer_base, outline_inner_base, static_cast<u32>(_path0.size()));
			}
		}

		if (aa)
		{
			const vector_t<vec2f_t>& outermost_path = (paint.filled && out) ? _path1 : _path0;
			const u32				 outermost_base = out ? outline_outer_base : fill_vtx_base;

			vg_path_expand(_path2, outermost_path, paint.aa_thickness);
			const u32 aa_base = db->vertex_count;
			emit_path_alpha(db, _path2, outermost_base, 0.0f, min, max);
			emit_strip_indices(db, aa_base, outermost_base, static_cast<u32>(outermost_path.size()));
		}
	}

	void vg_canvas_t::add_line(const vec2f_t& p0, const vec2f_t& p1, const vg_line_paint_t& paint, u32 draw_order, void* user_data)
	{
		vg_draw_buffer_t* db = get_draw_buffer(draw_order, user_data, nullptr);

		const vec2f_t dir = (p1 - p0).normalized();
		const vec2f_t n	  = {-dir.y, dir.x};
		const f32	  ht  = paint.thickness * 0.5f;
		const vec2f_t off = {n.x * ht, n.y * ht};

		const vec2f_t bb_min = {math::min(p0.x, p1.x) - paint.thickness, math::min(p0.y, p1.y) - paint.thickness};
		const vec2f_t bb_max = {math::max(p0.x, p1.x) + paint.thickness, math::max(p0.y, p1.y) + paint.thickness};

		const u32 base = db->vertex_count;
		_path0.resize(4);
		_path0[0] = {p0.x - off.x, p0.y - off.y};
		_path0[1] = {p1.x - off.x, p1.y - off.y};
		_path0[2] = {p1.x + off.x, p1.y + off.y};
		_path0[3] = {p0.x + off.x, p0.y + off.y};

		emit_path_solid(db, _path0, paint.color, bb_min, bb_max);
		emit_quad_indices(db, base);

		if (paint.aa_thickness > 0.0f)
		{
			vg_path_expand(_path2, _path0, paint.aa_thickness);
			const u32 aa_base = db->vertex_count;
			emit_path_alpha(db, _path2, base, 0.0f, bb_min, bb_max);
			emit_strip_indices(db, aa_base, base, 4);
		}
	}

	void vg_canvas_t::add_circle(const vec2f_t& center, f32 radius, const vg_circle_paint_t& paint, u32 draw_order, void* user_data)
	{
		vg_draw_buffer_t* db = get_draw_buffer(draw_order, user_data, nullptr);

		const vec2f_t bb_min = {center.x - radius - paint.thickness, center.y - radius - paint.thickness};
		const vec2f_t bb_max = {center.x + radius + paint.thickness, center.y + radius + paint.thickness};

		vg_path_circle(_path0, center, radius, paint.segments);

		const u32 base = db->vertex_count;
		emit_path_solid(db, _path0, paint.color, bb_min, bb_max);

		if (paint.filled)
		{
			const u32 center_idx = db->vertex_count;
			emit_central_solid(db, paint.color, bb_min, bb_max);
			emit_fan_indices(db, base, center_idx, paint.segments);

			if (paint.aa_thickness > 0.0f)
			{
				vg_path_expand(_path2, _path0, paint.aa_thickness);
				const u32 aa_base = db->vertex_count;
				emit_path_alpha(db, _path2, base, 0.0f, bb_min, bb_max);
				emit_strip_indices(db, aa_base, base, paint.segments);
			}
		}
		else
		{
			vg_path_expand(_path1, _path0, -paint.thickness);
			const u32 inner_base = db->vertex_count;
			emit_path_solid(db, _path1, paint.color, bb_min, bb_max);
			emit_strip_indices(db, base, inner_base, paint.segments);

			if (paint.aa_thickness > 0.0f)
			{
				vg_path_expand(_path2, _path0, paint.aa_thickness);
				const u32 aa_base = db->vertex_count;
				emit_path_alpha(db, _path2, base, 0.0f, bb_min, bb_max);
				emit_strip_indices(db, aa_base, base, paint.segments);
			}
		}
	}

	vec2f_t vg_canvas_t::measure_text(const char* text, size_t len, const vg_text_paint_t& paint)
	{
		if (!paint.font || !text || len == 0)
			return {0.0f, 0.0f};

		const font_data_t* fnt	   = paint.font;
		const f32		   scale   = paint.scale * fnt->scale;
		const f32		   spacing = static_cast<f32>(paint.spacing) * scale;

		f32 total_x = 0.0f;
		for (size_t i = 0; i < len; ++i)
		{
			const u8			c = static_cast<u8>(text[i]);
			const font_glyph_t& g = fnt->glyph_info[c];
			total_x += g.advance_x * scale;

			if (i + 1 < len)
			{
				const u8 c1 = static_cast<u8>(text[i + 1]);
				total_x += g.kern_advance[c1] * scale;
			}
			total_x += spacing;
		}

		const f32 height = (static_cast<f32>(fnt->ascent) - static_cast<f32>(fnt->descent)) * scale;
		return {total_x - spacing, height};
	}

	void vg_canvas_t::add_text(const char* text, size_t len, const vec2f_t& pos, const vg_text_paint_t& paint, u32 draw_order, void* user_data, bool use_cache)
	{
		if (!paint.font)
		{
			SFG_ERR("vg_canvas: add_text called without font");
			return;
		}
		if (len == 0)
			return;

		vg_draw_buffer_t* db = get_draw_buffer(draw_order, user_data, paint.font);

		if (use_cache)
		{
			const u64 hash = hash_text(text, len, paint);
			for (const text_cache_entry_t& e : _text_cache)
			{
				if (e.hash != hash)
					continue;

				const u32	 vtx_base = db->vertex_count;
				vg_vertex_t* verts	  = take_vertices(db, e.vtx_count);
				vg_index_t*	 indices  = take_indices(db, e.idx_count);
				std::memcpy(verts, &_text_cache_vertex_buffer[e.vtx_start], e.vtx_count * sizeof(vg_vertex_t));
				std::memcpy(indices, &_text_cache_index_buffer[e.idx_start], e.idx_count * sizeof(vg_index_t));

				for (u32 i = 0; i < e.vtx_count; ++i)
				{
					verts[i].pos.x += pos.x;
					verts[i].pos.y += pos.y;
				}
				for (u32 i = 0; i < e.idx_count; ++i)
					indices[i] = static_cast<vg_index_t>(indices[i] + vtx_base);
				return;
			}
		}

		const font_data_t* fnt		= paint.font;
		const f32		   scale	= paint.scale * fnt->scale;
		const f32		   spacing	= static_cast<f32>(paint.spacing) * scale;
		const f32		   subpixel = (fnt->kind == font_kind_e::lcd) ? 3.0f : 1.0f;

		const u32 char_count = static_cast<u32>(len);
		const u32 vtx_base	 = db->vertex_count;

		vg_vertex_t* verts	 = take_vertices(db, char_count * 4);
		vg_index_t*	 indices = take_indices(db, char_count * 6);

		const vec2f_t pen_origin = use_cache ? vec2f_t{0.0f, 0.0f} : pos;
		vec2f_t		  pen		 = {pen_origin.x, pen_origin.y + static_cast<f32>(fnt->ascent) * scale};

		u32 emitted_chars = 0;
		u32 prev		  = 0;

		for (size_t i = 0; i < len; ++i)
		{
			const u8			c = static_cast<u8>(text[i]);
			const font_glyph_t& g = fnt->glyph_info[c];

			if (prev != 0)
				pen.x += static_cast<f32>(fnt->glyph_info[prev].kern_advance[c]) * scale;

			const f32 quad_left	  = pen.x + (g.x_offset / subpixel) * paint.scale;
			const f32 quad_top	  = pen.y + g.y_offset * paint.scale;
			const f32 quad_right  = quad_left + g.width * paint.scale;
			const f32 quad_bottom = quad_top + g.height * paint.scale;

			vg_vertex_t& v0 = verts[emitted_chars * 4 + 0];
			vg_vertex_t& v1 = verts[emitted_chars * 4 + 1];
			vg_vertex_t& v2 = verts[emitted_chars * 4 + 2];
			vg_vertex_t& v3 = verts[emitted_chars * 4 + 3];

			v0.pos = {quad_left, quad_top};
			v1.pos = {quad_right, quad_top};
			v2.pos = {quad_right, quad_bottom};
			v3.pos = {quad_left, quad_bottom};

			v0.color = paint.color;
			v1.color = paint.color;
			v2.color = paint.color;
			v3.color = paint.color;

			const f32 ux = g.uv_x;
			const f32 uy = g.uv_y;
			const f32 uw = g.uv_w;
			const f32 uh = g.uv_h;

			if (paint.flip_uv)
			{
				v0.uv = {ux, uy + uh};
				v1.uv = {ux + uw, uy + uh};
				v2.uv = {ux + uw, uy};
				v3.uv = {ux, uy};
			}
			else
			{
				v0.uv = {ux, uy};
				v1.uv = {ux + uw, uy};
				v2.uv = {ux + uw, uy + uh};
				v3.uv = {ux, uy + uh};
			}

			const u32 base				   = vtx_base + emitted_chars * 4;
			indices[emitted_chars * 6 + 0] = static_cast<vg_index_t>(base + 0);
			indices[emitted_chars * 6 + 1] = static_cast<vg_index_t>(base + 1);
			indices[emitted_chars * 6 + 2] = static_cast<vg_index_t>(base + 3);
			indices[emitted_chars * 6 + 3] = static_cast<vg_index_t>(base + 1);
			indices[emitted_chars * 6 + 4] = static_cast<vg_index_t>(base + 2);
			indices[emitted_chars * 6 + 5] = static_cast<vg_index_t>(base + 3);

			pen.x += g.advance_x * scale + spacing;
			emitted_chars++;
			prev = c;
		}

		if (use_cache)
		{
			const u32 vtx_count = char_count * 4;
			const u32 idx_count = char_count * 6;

			if (_text_cache_vertex_count + vtx_count <= _text_cache_vertex_capacity && _text_cache_index_count + idx_count <= _text_cache_index_capacity)
			{
				text_cache_entry_t e;
				e.hash		= hash_text(text, len, paint);
				e.vtx_start = _text_cache_vertex_count;
				e.vtx_count = vtx_count;
				e.idx_start = _text_cache_index_count;
				e.idx_count = idx_count;

				std::memcpy(&_text_cache_vertex_buffer[_text_cache_vertex_count], verts, vtx_count * sizeof(vg_vertex_t));
				vg_index_t* cache_idx = &_text_cache_index_buffer[_text_cache_index_count];
				for (u32 i = 0; i < idx_count; ++i)
					cache_idx[i] = static_cast<vg_index_t>(indices[i] - vtx_base);

				_text_cache_vertex_count += vtx_count;
				_text_cache_index_count += idx_count;
				_text_cache.push_back(e);
			}

			for (u32 i = 0; i < vtx_count; ++i)
			{
				verts[i].pos.x += pos.x;
				verts[i].pos.y += pos.y;
			}
		}
	}

	void vg_canvas_t::clear_text_cache()
	{
		_text_cache.resize(0);
		_text_cache_vertex_count = 0;
		_text_cache_index_count	 = 0;
	}
}
