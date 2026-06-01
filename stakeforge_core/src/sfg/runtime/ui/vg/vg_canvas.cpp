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
#include <sfg/common/hashing.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>
#include <sfg/runtime/resources/atlas.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

#include <algorithm>
#include <cstring>

namespace sfg::ui
{
	namespace
	{
		struct text_bounds_t
		{
			f32	 min_x	   = 0.0f;
			f32	 min_y	   = 0.0f;
			f32	 max_x	   = 0.0f;
			f32	 max_y	   = 0.0f;
			f32	 advance_x = 0.0f;
			bool valid	   = false;
		};

		inline f32 snap_px(f32 v)
		{
			return math::round(v);
		}

		inline f32 snap_size_px(f32 v)
		{
			return math::max(0.0f, math::round(v));
		}

		inline f32 snap_thickness_px(f32 v)
		{
			return v > 0.0f ? math::max(1.0f, math::round(v)) : 0.0f;
		}

		inline void snap_rect_px(const vec2f_t& in_min, const vec2f_t& in_max, vec2f_t& out_min, vec2f_t& out_max)
		{
			out_min = {snap_px(in_min.x), snap_px(in_min.y)};
			out_max = {out_min.x + snap_size_px(in_max.x - in_min.x), out_min.y + snap_size_px(in_max.y - in_min.y)};
		}

		text_bounds_t measure_text_bounds(const char* text, size_t len, glyph_atlas_t& atlas, const font_runtime_t* font, const size_metrics_t& metrics, u32 px, glyph_raster_mode_e raster_mode, f32 scale, f32 spacing)
		{
			text_bounds_t bounds = {};
			vec2f_t		  pen	 = {0.0f, metrics.ascent_px * scale};
			u32			  prev	 = 0;

			for (size_t i = 0; i < len; ++i)
			{
				const u32 c = static_cast<u8>(text[i]);
				if (prev != 0)
					pen.x += atlas.get_kern_advance(font, prev, c, px) * scale;

				const glyph_entry_t* g = atlas.request_glyph(font, c, px, raster_mode);

				if (g->width > 0 && g->height > 0)
				{
					const f32 quad_left	  = pen.x + g->left_bearing * scale;
					const f32 quad_top	  = pen.y + g->top_bearing * scale;
					const f32 quad_right  = quad_left + static_cast<f32>(g->width) * scale;
					const f32 quad_bottom = quad_top + static_cast<f32>(g->height) * scale;

					if (!bounds.valid)
					{
						bounds.min_x = quad_left;
						bounds.min_y = quad_top;
						bounds.max_x = quad_right;
						bounds.max_y = quad_bottom;
						bounds.valid = true;
					}
					else
					{
						bounds.min_x = math::min(bounds.min_x, quad_left);
						bounds.min_y = math::min(bounds.min_y, quad_top);
						bounds.max_x = math::max(bounds.max_x, quad_right);
						bounds.max_y = math::max(bounds.max_y, quad_bottom);
					}
				}

				pen.x += g->advance_x * scale + spacing;
				prev = c;
			}

			bounds.advance_x = pen.x - spacing;
			if (!bounds.valid)
			{
				bounds.max_x = math::max(0.0f, bounds.advance_x);
				bounds.max_y = metrics.line_height_px * scale;
			}
			return bounds;
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

	}

	void vg_canvas_t::emit_path_solid(vg_draw_buffer_t* db, span_t<const vec2f_t> path, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max)
	{
		const f32	 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
		const f32	 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;
		vg_vertex_t* v	   = take_vertices(db, static_cast<u32>(path.size));
		for (size_t i = 0; i < path.size; ++i)
		{
			v[i].pos   = path.data[i];
			v[i].color = color;
			v[i].uv.x  = (path.data[i].x - min.x) * inv_x;
			v[i].uv.y  = (path.data[i].y - min.y) * inv_y;
		}
	}

	void vg_canvas_t::emit_path_grad(vg_draw_buffer_t* db, span_t<const vec2f_t> path, const vec4f_t& color_a, const vec4f_t& color_b, vg_gradient_e dir, const vec2f_t& min, const vec2f_t& max)
	{
		const f32 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
		const f32 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;

		vg_vertex_t*  v	   = take_vertices(db, static_cast<u32>(path.size));
		const vec4f_t diff = color_b - color_a;

		for (size_t i = 0; i < path.size; ++i)
		{
			const f32 ux = (path.data[i].x - min.x) * inv_x;
			const f32 uy = (path.data[i].y - min.y) * inv_y;
			const f32 t	 = (dir == vg_gradient_e::horizontal) ? ux : uy;
			v[i].pos	 = path.data[i];
			v[i].color.x = color_a.x + diff.x * t;
			v[i].color.y = color_a.y + diff.y * t;
			v[i].color.z = color_a.z + diff.z * t;
			v[i].color.w = color_a.w + diff.w * t;
			v[i].uv.x	 = ux;
			v[i].uv.y	 = uy;
		}
	}

	void vg_canvas_t::emit_central_solid(vg_draw_buffer_t* db, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max)
	{
		vg_vertex_t* v = take_vertices(db, 1);
		v[0].pos	   = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
		v[0].uv		   = {0.5f, 0.5f};
		v[0].color	   = color;
	}

	void vg_canvas_t::emit_central_grad(vg_draw_buffer_t* db, const vec4f_t& color_a, const vec4f_t& color_b, const vec2f_t& min, const vec2f_t& max)
	{
		vg_vertex_t* v = take_vertices(db, 1);
		v[0].pos	   = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
		v[0].uv		   = {0.5f, 0.5f};
		v[0].color	   = (color_a + color_b) * 0.5f;
	}

	void vg_canvas_t::emit_path_alpha(vg_draw_buffer_t* db, span_t<const vec2f_t> path, u32 source_vtx_base, f32 alpha, const vec2f_t& min, const vec2f_t& max)
	{
		const f32 inv_x = (max.x - min.x) > 0.0f ? 1.0f / (max.x - min.x) : 0.0f;
		const f32 inv_y = (max.y - min.y) > 0.0f ? 1.0f / (max.y - min.y) : 0.0f;

		vg_vertex_t* v = take_vertices(db, static_cast<u32>(path.size));
		for (size_t i = 0; i < path.size; ++i)
		{
			v[i].pos	 = path.data[i];
			v[i].color	 = db->vertex_start[source_vtx_base + i].color;
			v[i].color.w = alpha;
			v[i].uv.x	 = (path.data[i].x - min.x) * inv_x;
			v[i].uv.y	 = (path.data[i].y - min.y) * inv_y;
		}
	}

	void vg_canvas_t::emit_quad_indices(vg_draw_buffer_t* db, u32 base)
	{
		vg_index_t* idx = take_indices(db, 6);
		idx[0]			= static_cast<vg_index_t>(base + 0);
		idx[1]			= static_cast<vg_index_t>(base + 1);
		idx[2]			= static_cast<vg_index_t>(base + 3);
		idx[3]			= static_cast<vg_index_t>(base + 1);
		idx[4]			= static_cast<vg_index_t>(base + 2);
		idx[5]			= static_cast<vg_index_t>(base + 3);
	}

	void vg_canvas_t::emit_fan_indices(vg_draw_buffer_t* db, u32 ring_base, u32 center_idx, u32 ring_size)
	{
		vg_index_t* idx = take_indices(db, ring_size * 3);
		for (u32 i = 0; i < ring_size; ++i)
		{
			idx[i * 3 + 0] = static_cast<vg_index_t>(center_idx);
			idx[i * 3 + 1] = static_cast<vg_index_t>(ring_base + i);
			idx[i * 3 + 2] = static_cast<vg_index_t>(ring_base + ((i + 1) % ring_size));
		}
	}

	void vg_canvas_t::emit_strip_indices(vg_draw_buffer_t* db, u32 outer_base, u32 inner_base, u32 ring_size)
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

	void vg_canvas_t::emit_open_strip_indices(vg_draw_buffer_t* db, u32 outer_base, u32 inner_base, u32 ring_size)
	{
		if (ring_size < 2)
			return;

		vg_index_t* idx = take_indices(db, (ring_size - 1) * 6);
		for (u32 i = 0; i < ring_size - 1; ++i)
		{
			const u32 o0   = outer_base + i;
			const u32 o1   = outer_base + i + 1;
			const u32 in0  = inner_base + i;
			const u32 in1  = inner_base + i + 1;
			const u32 base = i * 6;
			idx[base + 0]  = static_cast<vg_index_t>(o0);
			idx[base + 1]  = static_cast<vg_index_t>(o1);
			idx[base + 2]  = static_cast<vg_index_t>(in0);
			idx[base + 3]  = static_cast<vg_index_t>(o1);
			idx[base + 4]  = static_cast<vg_index_t>(in1);
			idx[base + 5]  = static_cast<vg_index_t>(in0);
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
		_scissor_clip_stack.reserve(cfg.clip_stack_capacity);
		_cpu_clip_stack.reserve(cfg.clip_stack_capacity);
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
		_scissor_clip_stack.resize(0);
		_cpu_clip_stack.resize(0);
		_text_cache.resize(0);
		_path0.resize(0);
		_path1.resize(0);
		_path2.resize(0);
	}

	namespace
	{
		gfx_shader_handle resolve_shader(resource_handle_t handle)
		{
			if (handle == 0)
				return {};
			const shader_internals_t* internals = resource_manager_t::get().find_internals<shader_internals_t>(handle);
			if (internals == nullptr)
				return {};
			return internals->find_pso(0);
		}

		gpu_index_t resolve_texture_index(resource_handle_t handle)
		{
			if (handle == NULL_RESOURCE_HANDLE)
				return NULL_GPU_INDEX;
			const texture_internals_t* internals = resource_manager_t::get().find_internals<texture_internals_t>(handle);
			if (internals == nullptr || internals->texture.is_null())
				return NULL_GPU_INDEX;
			return gfx_backend::get().get_texture_gpu_index(internals->texture, 0);
		}

	}

	void vg_canvas_t::resolve()
	{
		for (vg_draw_buffer_t& db : _draw_buffers)
		{
			SFG_ASSERT(db.state.pipeline != NULL_RESOURCE_HANDLE);
			const gfx_shader_handle p = resolve_shader(db.state.pipeline);
			SFG_ASSERT(!p.is_null());
			db.resolved.pipeline = p;

			for (u8 i = 0; i < 4; ++i)
			{
				const ui_resource_ref_t& ref  = db.state.constants[i];
				db.resolved.constant_types[i] = ref.type;
				if (ref.type == ui_resource_type_e::gpu_index)
				{
					db.resolved.constants[i] = ref.gpu_indices[0];
				}
				else if (ref.type == ui_resource_type_e::gpu_index_fof)
				{
					db.resolved.constants[i] = ref.gpu_indices[0];
					for (u8 j = 0; j < BACK_BUFFER_COUNT; ++j)
						db.resolved.constant_frames[i][j] = ref.gpu_indices[j];
				}
				else if (ref.type == ui_resource_type_e::texture)
				{
					const gpu_index_t idx = resolve_texture_index(ref.handle);
					SFG_ASSERT(idx != NULL_GPU_INDEX);
					db.resolved.constants[i] = idx;
				}
				else
					db.resolved.constants[i] = NULL_GPU_INDEX;
			}
		}
	}

	void vg_canvas_t::frame_begin(const vec4f_t& screen_clip)
	{
		_draw_buffers.resize(0);
		_scissor_clip_stack.resize(0);
		_cpu_clip_stack.resize(0);
		_buffer_counter = 0;
		_scissor_clip_stack.push_back({screen_clip});
		_cpu_clip_stack.push_back({screen_clip});
	}

	void vg_canvas_t::frame_end()
	{
		std::stable_sort(_draw_buffers.begin(), _draw_buffers.end(), [](const vg_draw_buffer_t& a, const vg_draw_buffer_t& b) { return a.draw_order < b.draw_order; });
	}

	void vg_canvas_t::push_clip(const vec4f_t& rect, clip_mode_e mode)
	{
		if (mode == clip_mode_e::cpu_rect)
		{
			const vec4f_t base = current_cpu_clip();
			_cpu_clip_stack.push_back({intersect_clip(base, rect)});
		}
		else if (mode == clip_mode_e::scissor_rect)
		{
			const vec4f_t base = current_scissor_clip();
			_scissor_clip_stack.push_back({intersect_clip(base, rect)});
		}
	}

	void vg_canvas_t::pop_clip(clip_mode_e mode)
	{
		if (mode == clip_mode_e::cpu_rect)
		{
			SFG_ASSERT(_cpu_clip_stack.size() > 1);
			_cpu_clip_stack.pop_back();
		}
		else if (mode == clip_mode_e::scissor_rect)
		{
			SFG_ASSERT(_scissor_clip_stack.size() > 1);
			_scissor_clip_stack.pop_back();
		}
	}

	vec4f_t vg_canvas_t::current_scissor_clip() const
	{
		SFG_ASSERT(!_scissor_clip_stack.empty());
		return _scissor_clip_stack.back().rect;
	}

	vec4f_t vg_canvas_t::current_cpu_clip() const
	{
		SFG_ASSERT(!_cpu_clip_stack.empty());
		return _cpu_clip_stack.back().rect;
	}

	bool vg_canvas_t::has_cpu_clip() const
	{
		return _cpu_clip_stack.size() > 1;
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

	bool vg_canvas_t::clip_rect_to_cpu(vec2f_t& min, vec2f_t& max) const
	{
		if (!has_cpu_clip())
			return true;

		const vec4f_t clip = current_cpu_clip();
		min.x			   = math::max(min.x, clip.x);
		min.y			   = math::max(min.y, clip.y);
		max.x			   = math::min(max.x, clip.x + clip.z);
		max.y			   = math::min(max.y, clip.y + clip.w);
		return max.x > min.x && max.y > min.y;
	}

	bool vg_canvas_t::clip_line_to_cpu(vec2f_t& p0, vec2f_t& p1, f32 thickness) const
	{
		if (!has_cpu_clip())
			return true;

		const vec4f_t clip	 = current_cpu_clip();
		const vec2f_t bb_min = {math::min(p0.x, p1.x) - thickness, math::min(p0.y, p1.y) - thickness};
		const vec2f_t bb_max = {math::max(p0.x, p1.x) + thickness, math::max(p0.y, p1.y) + thickness};
		if (bb_max.x <= clip.x || bb_max.y <= clip.y || bb_min.x >= clip.x + clip.z || bb_min.y >= clip.y + clip.w)
			return false;

		const vec2f_t delta = p1 - p0;
		f32			  t0	= 0.0f;
		f32			  t1	= 1.0f;

		const auto clip_axis = [&](f32 p, f32 q) {
			if (p == 0.0f)
				return q >= 0.0f;
			const f32 r = q / p;
			if (p < 0.0f)
			{
				if (r > t1)
					return false;
				if (r > t0)
					t0 = r;
			}
			else
			{
				if (r < t0)
					return false;
				if (r < t1)
					t1 = r;
			}
			return true;
		};

		if (!clip_axis(-delta.x, p0.x - clip.x))
			return false;
		if (!clip_axis(delta.x, clip.x + clip.z - p0.x))
			return false;
		if (!clip_axis(-delta.y, p0.y - clip.y))
			return false;
		if (!clip_axis(delta.y, clip.y + clip.w - p0.y))
			return false;

		const vec2f_t original_p0 = p0;
		p0						  = original_p0 + delta * t0;
		p1						  = original_p0 + delta * t1;
		return true;
	}

	vg_draw_buffer_t* vg_canvas_t::get_draw_buffer(u32 draw_order, const ui_render_state_t& state)
	{
		const vec4f_t clip = current_scissor_clip();

		for (vg_draw_buffer_t& db : _draw_buffers)
		{
			if (db.draw_order != draw_order)
				continue;
			if (db.state.pipeline != state.pipeline)
				continue;
			if (SFG_MEMCMP(db.state.constants, state.constants, sizeof(state.constants)) != 0)
				continue;
			if (!db.clip.equals(clip, 0.5f))
				continue;
			return &db;
		}

		SFG_ASSERT(_buffer_counter < _buffer_count);

		vg_draw_buffer_t db = {};
		db.vertex_start		= _vertex_pool + _buffer_counter * _vertex_capacity_per_buffer;
		db.index_start		= _index_pool + _buffer_counter * _index_capacity_per_buffer;
		db.vertex_capacity	= _vertex_capacity_per_buffer;
		db.index_capacity	= _index_capacity_per_buffer;
		db.clip				= clip;
		db.draw_order		= draw_order;
		db.state			= state;

		_buffer_counter++;
		_draw_buffers.push_back(db);
		return &_draw_buffers.back();
	}

	void vg_canvas_t::add_rect(const vec2f_t& min, const vec2f_t& max, const vg_rect_paint_t& paint, const ui_render_state_t& state, u32 draw_order)
	{
		const bool round = paint.rounding > 0.0f;
		const bool out	 = paint.outline_thickness > 0.0f;
		const bool aa	 = paint.aa_thickness > 0.0f;
		const bool grad	 = paint.gradient != vg_gradient_e::none;
		vec2f_t	   draw_min;
		vec2f_t	   draw_max;

		if (!paint.filled && !out)
			return;

		snap_rect_px(min, max, draw_min, draw_max);

		vec2f_t clipped_min = draw_min;
		vec2f_t clipped_max = draw_max;
		if (!clip_rect_to_cpu(clipped_min, clipped_max))
			return;

		if (!round && !out && !aa)
		{
			vg_draw_buffer_t* db	   = get_draw_buffer(draw_order, state);
			const u32		  vtx_base = db->vertex_count;
			vec2f_t			  path[4]  = {{clipped_min.x, clipped_min.y}, {clipped_max.x, clipped_min.y}, {clipped_max.x, clipped_max.y}, {clipped_min.x, clipped_max.y}};
			if (grad)
				emit_path_grad(db, {path, 4}, paint.fill_color_a, paint.fill_color_b, paint.gradient, draw_min, draw_max);
			else
				emit_path_solid(db, {path, 4}, paint.fill_color_a, draw_min, draw_max);
			emit_quad_indices(db, vtx_base);
			return;
		}

		vg_draw_buffer_t* db = get_draw_buffer(draw_order, state);

		if (round)
			vg_path_rounded_rect(_path0, draw_min, draw_max, snap_size_px(paint.rounding), paint.rounding_segs);
		else
			vg_path_sharp_rect(_path0, draw_min, draw_max);

		if (out)
		{
			f32 thickness = snap_thickness_px(paint.outline_thickness);
			if (round)
			{
				const f32 dir = paint.filled ? 1.0f : -1.0f;
				vg_path_expand(_path1, _path0, thickness * dir);
			}
			else if (paint.filled)
			{
				const vec2f_t outline_min = {draw_min.x - thickness, draw_min.y - thickness};
				const vec2f_t outline_max = {draw_max.x + thickness, draw_max.y + thickness};
				vg_path_sharp_rect(_path1, outline_min, outline_max);
			}
			else
			{
				const f32 max_thickness	  = math::min((draw_max.x - draw_min.x) * 0.5f, (draw_max.y - draw_min.y) * 0.5f);
				thickness				  = math::min(thickness, max_thickness);
				const vec2f_t outline_min = {draw_min.x + thickness, draw_min.y + thickness};
				const vec2f_t outline_max = {draw_max.x - thickness, draw_max.y - thickness};
				vg_path_sharp_rect(_path1, outline_min, outline_max);
			}
		}

		const u32 fill_vtx_base = db->vertex_count;
		if (paint.filled)
		{
			if (grad)
				emit_path_grad(db, {_path0.data(), _path0.size()}, paint.fill_color_a, paint.fill_color_b, paint.gradient, draw_min, draw_max);
			else
				emit_path_solid(db, {_path0.data(), _path0.size()}, paint.fill_color_a, draw_min, draw_max);

			if (round)
			{
				const u32 center_idx = db->vertex_count;
				if (grad)
					emit_central_grad(db, paint.fill_color_a, paint.fill_color_b, draw_min, draw_max);
				else
					emit_central_solid(db, paint.fill_color_a, draw_min, draw_max);
				emit_fan_indices(db, fill_vtx_base, center_idx, static_cast<u32>(_path0.size()));
			}
			else
			{
				emit_quad_indices(db, fill_vtx_base);
			}
		}

		u32 outline_outer_base = UINT32_MAX;
		if (out)
		{
			outline_outer_base = db->vertex_count;
			if (paint.filled)
			{
				emit_path_solid(db, {_path1.data(), _path1.size()}, paint.outline_color, draw_min, draw_max);
				const u32 outline_inner_base = db->vertex_count;
				emit_path_solid(db, {_path0.data(), _path0.size()}, paint.outline_color, draw_min, draw_max);
				emit_strip_indices(db, outline_outer_base, outline_inner_base, static_cast<u32>(_path0.size()));
			}
			else
			{
				emit_path_solid(db, {_path0.data(), _path0.size()}, paint.outline_color, draw_min, draw_max);
				const u32 outline_inner_base = db->vertex_count;
				emit_path_solid(db, {_path1.data(), _path1.size()}, paint.outline_color, draw_min, draw_max);
				emit_strip_indices(db, outline_outer_base, outline_inner_base, static_cast<u32>(_path0.size()));
			}
		}

		if (aa)
		{
			const vector_t<vec2f_t>& outermost_path = (paint.filled && out) ? _path1 : _path0;
			const u32				 outermost_base = out ? outline_outer_base : fill_vtx_base;

			vg_path_expand(_path2, outermost_path, paint.aa_thickness);
			const u32 aa_base = db->vertex_count;
			emit_path_alpha(db, {_path2.data(), _path2.size()}, outermost_base, 0.0f, draw_min, draw_max);
			emit_strip_indices(db, aa_base, outermost_base, static_cast<u32>(outermost_path.size()));
		}
	}

	void vg_canvas_t::add_line(const vec2f_t& p0, const vec2f_t& p1, const vg_line_paint_t& paint, const ui_render_state_t& state, u32 draw_order)
	{
		vec2f_t clipped_p0 = p0;
		vec2f_t clipped_p1 = p1;
		if (!clip_line_to_cpu(clipped_p0, clipped_p1, paint.thickness))
			return;

		vg_draw_buffer_t* db = get_draw_buffer(draw_order, state);

		const vec2f_t dir = (clipped_p1 - clipped_p0).normalized();
		const vec2f_t n	  = {-dir.y, dir.x};
		const f32	  ht  = paint.thickness * 0.5f;
		const vec2f_t off = {n.x * ht, n.y * ht};

		const vec2f_t bb_min = {math::min(clipped_p0.x, clipped_p1.x) - paint.thickness, math::min(clipped_p0.y, clipped_p1.y) - paint.thickness};
		const vec2f_t bb_max = {math::max(clipped_p0.x, clipped_p1.x) + paint.thickness, math::max(clipped_p0.y, clipped_p1.y) + paint.thickness};

		const u32 base = db->vertex_count;
		_path0.resize(4);
		_path0[0] = {clipped_p0.x - off.x, clipped_p0.y - off.y};
		_path0[1] = {clipped_p1.x - off.x, clipped_p1.y - off.y};
		_path0[2] = {clipped_p1.x + off.x, clipped_p1.y + off.y};
		_path0[3] = {clipped_p0.x + off.x, clipped_p0.y + off.y};

		emit_path_solid(db, {_path0.data(), _path0.size()}, paint.color, bb_min, bb_max);
		emit_quad_indices(db, base);

		if (paint.aa_thickness > 0.0f)
		{
			vg_path_expand(_path2, _path0, paint.aa_thickness);
			const u32 aa_base = db->vertex_count;
			emit_path_alpha(db, {_path2.data(), _path2.size()}, base, 0.0f, bb_min, bb_max);
			emit_strip_indices(db, aa_base, base, 4);
		}
	}

	void vg_canvas_t::add_circle(const vec2f_t& center, f32 radius, const vg_circle_paint_t& paint, const ui_render_state_t& state, u32 draw_order)
	{
		const vec2f_t bb_min	  = {center.x - radius - paint.thickness, center.y - radius - paint.thickness};
		const vec2f_t bb_max	  = {center.x + radius + paint.thickness, center.y + radius + paint.thickness};
		vec2f_t		  clipped_min = bb_min;
		vec2f_t		  clipped_max = bb_max;
		if (!clip_rect_to_cpu(clipped_min, clipped_max))
			return;

		vg_draw_buffer_t* db = get_draw_buffer(draw_order, state);

		vg_path_circle(_path0, center, radius, paint.segments);

		const u32 base = db->vertex_count;
		emit_path_solid(db, {_path0.data(), _path0.size()}, paint.color, bb_min, bb_max);

		if (paint.filled)
		{
			const u32 center_idx = db->vertex_count;
			emit_central_solid(db, paint.color, bb_min, bb_max);
			emit_fan_indices(db, base, center_idx, paint.segments);

			if (paint.aa_thickness > 0.0f)
			{
				vg_path_expand(_path2, _path0, paint.aa_thickness);
				const u32 aa_base = db->vertex_count;
				emit_path_alpha(db, {_path2.data(), _path2.size()}, base, 0.0f, bb_min, bb_max);
				emit_strip_indices(db, aa_base, base, paint.segments);
			}
		}
		else
		{
			vg_path_expand(_path1, _path0, -paint.thickness);
			const u32 inner_base = db->vertex_count;
			emit_path_solid(db, {_path1.data(), _path1.size()}, paint.color, bb_min, bb_max);
			emit_strip_indices(db, base, inner_base, paint.segments);

			if (paint.aa_thickness > 0.0f)
			{
				vg_path_expand(_path2, _path0, paint.aa_thickness);
				const u32 aa_base = db->vertex_count;
				emit_path_alpha(db, {_path2.data(), _path2.size()}, base, 0.0f, bb_min, bb_max);
				emit_strip_indices(db, aa_base, base, paint.segments);
			}
		}
	}

	void vg_canvas_t::add_arc(const vec2f_t& center, f32 radius, f32 start, f32 end, const vg_arc_paint_t& paint, const ui_render_state_t& state, u32 draw_order)
	{
		if (radius <= 0.0f || paint.thickness <= 0.0f)
			return;

		const f32 half_thickness  = paint.thickness * 0.5f;
		const f32 outer_radius	  = radius + half_thickness;
		const f32 inner_radius	  = math::max(0.0f, radius - half_thickness);
		const f32 outer_aa_radius = outer_radius + paint.aa_thickness;
		const f32 inner_aa_radius = math::max(0.0f, inner_radius - paint.aa_thickness);

		const vec2f_t bb_min	  = {center.x - outer_aa_radius, center.y - outer_aa_radius};
		const vec2f_t bb_max	  = {center.x + outer_aa_radius, center.y + outer_aa_radius};
		vec2f_t		  clipped_min = bb_min;
		vec2f_t		  clipped_max = bb_max;
		if (!clip_rect_to_cpu(clipped_min, clipped_max))
			return;

		vg_path_arc(_path0, center, outer_radius, start, end, paint.segments);
		vg_path_arc(_path1, center, inner_radius, start, end, paint.segments);

		vg_draw_buffer_t* db		 = get_draw_buffer(draw_order, state);
		const u32		  outer_base = db->vertex_count;
		emit_path_solid(db, {_path0.data(), _path0.size()}, paint.color, bb_min, bb_max);
		const u32 inner_base = db->vertex_count;
		emit_path_solid(db, {_path1.data(), _path1.size()}, paint.color, bb_min, bb_max);
		emit_open_strip_indices(db, outer_base, inner_base, static_cast<u32>(_path0.size()));

		const vec2f_t cap_start = _path0.front() + (_path1.front() - _path0.front()) * 0.5f;
		const vec2f_t cap_end	= _path0.back() + (_path1.back() - _path0.back()) * 0.5f;

		if (paint.aa_thickness > 0.0f)
		{
			vg_path_arc(_path2, center, outer_aa_radius, start, end, paint.segments);
			const u32 outer_aa_base = db->vertex_count;
			emit_path_alpha(db, {_path2.data(), _path2.size()}, outer_base, 0.0f, bb_min, bb_max);
			emit_open_strip_indices(db, outer_aa_base, outer_base, static_cast<u32>(_path0.size()));

			vg_path_arc(_path2, center, inner_aa_radius, start, end, paint.segments);
			const u32 inner_aa_base = db->vertex_count;
			emit_path_alpha(db, {_path2.data(), _path2.size()}, inner_base, 0.0f, bb_min, bb_max);
			emit_open_strip_indices(db, inner_base, inner_aa_base, static_cast<u32>(_path1.size()));
		}

		vg_circle_paint_t cap = {};
		cap.color			  = paint.color;
		cap.filled			  = true;
		cap.segments		  = 24;
		cap.aa_thickness	  = paint.aa_thickness;
		add_circle(cap_start, half_thickness, cap, state, draw_order);
		add_circle(cap_end, half_thickness, cap, state, draw_order);
	}

	void vg_canvas_t::add_convex(span_t<const vec2f_t> path, const vg_convex_paint_t& paint, const ui_render_state_t& state, u32 draw_order)
	{
		if (path.size < 3)
			return;

		vec2f_t bb_min = path.data[0];
		vec2f_t bb_max = path.data[0];
		for (size_t i = 1; i < path.size; ++i)
		{
			bb_min = vec2f_t::min(bb_min, path.data[i]);
			bb_max = vec2f_t::max(bb_max, path.data[i]);
		}

		vec2f_t clipped_min = bb_min;
		vec2f_t clipped_max = bb_max;
		if (!clip_rect_to_cpu(clipped_min, clipped_max))
			return;

		vg_draw_buffer_t* db = get_draw_buffer(draw_order, state);

		const u32 base = db->vertex_count;
		if (paint.gradient != vg_gradient_e::none)
			emit_path_grad(db, path, paint.fill_color_a, paint.fill_color_b, paint.gradient, bb_min, bb_max);
		else
			emit_path_solid(db, path, paint.fill_color_a, bb_min, bb_max);

		vg_index_t* idx = take_indices(db, static_cast<u32>((path.size - 2) * 3));
		for (size_t i = 0; i < path.size - 2; ++i)
		{
			idx[i * 3 + 0] = static_cast<vg_index_t>(base);
			idx[i * 3 + 1] = static_cast<vg_index_t>(base + i + 1);
			idx[i * 3 + 2] = static_cast<vg_index_t>(base + i + 2);
		}

		if (paint.aa_thickness > 0.0f)
		{
			_path0.resize(path.size);
			for (size_t i = 0; i < path.size; ++i)
				_path0[i] = path.data[i];

			vg_path_expand(_path2, _path0, paint.aa_thickness);
			const u32 aa_base = db->vertex_count;
			emit_path_alpha(db, {_path2.data(), _path2.size()}, base, 0.0f, bb_min, bb_max);
			emit_strip_indices(db, aa_base, base, static_cast<u32>(path.size));
		}
	}

	vec2f_t vg_canvas_t::measure_text(const char* text, size_t len, const vg_text_paint_t& paint)
	{
		if (!paint.font || !text || len == 0)
			return {0.0f, 0.0f};

		const u32			 px		 = get_text_paint_raster_px(paint);
		glyph_atlas_t&		 atlas	 = resource_manager_t::get().get_glyph_atlas();
		const size_metrics_t metrics = atlas.request_size_metrics(paint.font, px);
		const f32			 scale	 = get_text_paint_draw_scale(paint, px);
		const f32			 spacing = paint.spacing;

		const text_bounds_t bounds = measure_text_bounds(text, len, atlas, paint.font, metrics, px, paint.raster_mode, scale, spacing);
		return {bounds.valid ? bounds.max_x - bounds.min_x : math::max(0.0f, bounds.advance_x), bounds.max_y - bounds.min_y};
	}

	void vg_canvas_t::add_text(const char* text, size_t len, const vec2f_t& pos, const vg_text_paint_t& paint, const ui_render_state_t& state, u32 draw_order, bool use_cache)
	{
		if (!paint.font)
		{
			SFG_ERR("vg_canvas: add_text called without font");
			return;
		}
		if (len == 0)
			return;

		const bool cpu_clip = has_cpu_clip();
		if (cpu_clip)
			use_cache = false;

		vg_draw_buffer_t* db		 = get_draw_buffer(draw_order, state);
		const vec2f_t	  draw_pos	 = {snap_px(pos.x), snap_px(pos.y)};
		u64				  cache_hash = 0;

		if (use_cache)
		{
			cache_hash = hashing_t::hash_u64_combine(hashing_t::hash_u64(text, len), paint.font, paint.color, paint.size_px, paint.raster_px, paint.spacing, paint.raster_mode, paint.flip_uv);
			for (const text_cache_entry_t& e : _text_cache)
			{
				if (e.hash != cache_hash)
					continue;

				const u32	 vtx_base = db->vertex_count;
				vg_vertex_t* verts	  = take_vertices(db, e.vtx_count);
				vg_index_t*	 indices  = take_indices(db, e.idx_count);
				SFG_MEMCPY(verts, &_text_cache_vertex_buffer[e.vtx_start], e.vtx_count * sizeof(vg_vertex_t));
				SFG_MEMCPY(indices, &_text_cache_index_buffer[e.idx_start], e.idx_count * sizeof(vg_index_t));

				for (u32 i = 0; i < e.vtx_count; ++i)
				{
					verts[i].pos.x += draw_pos.x;
					verts[i].pos.y += draw_pos.y;
				}
				for (u32 i = 0; i < e.idx_count; ++i)
					indices[i] = static_cast<vg_index_t>(indices[i] + vtx_base);
				return;
			}
		}

		const u32			 px		 = get_text_paint_raster_px(paint);
		glyph_atlas_t&		 atlas	 = resource_manager_t::get().get_glyph_atlas();
		const size_metrics_t metrics = atlas.request_size_metrics(paint.font, px);
		const f32			 scale	 = get_text_paint_draw_scale(paint, px);
		const f32			 spacing = paint.spacing;
		const text_bounds_t	 bounds	 = measure_text_bounds(text, len, atlas, paint.font, metrics, px, paint.raster_mode, scale, spacing);

		const u32 char_count = static_cast<u32>(len);
		const u32 vtx_base	 = db->vertex_count;
		const u32 idx_base	 = db->index_count;

		vg_vertex_t* verts	 = take_vertices(db, char_count * 4);
		vg_index_t*	 indices = take_indices(db, char_count * 6);

		const vec2f_t pen_origin = use_cache ? vec2f_t{0.0f, 0.0f} : draw_pos;
		vec2f_t		  pen		 = {pen_origin.x - bounds.min_x, pen_origin.y + metrics.ascent_px * scale - bounds.min_y};

		u32 emitted_chars = 0;
		u32 prev		  = 0;

		for (size_t i = 0; i < len; ++i)
		{
			const u32			 c = static_cast<u8>(text[i]);
			const glyph_entry_t* g = atlas.request_glyph(paint.font, c, px, paint.raster_mode);

			if (prev != 0)
				pen.x += atlas.get_kern_advance(paint.font, prev, c, px) * scale;

			const f32 quad_left	  = pen.x + g->left_bearing * scale;
			const f32 quad_top	  = pen.y + g->top_bearing * scale;
			const f32 quad_right  = quad_left + static_cast<f32>(g->width) * scale;
			const f32 quad_bottom = quad_top + static_cast<f32>(g->height) * scale;

			vec2f_t clipped_min = {quad_left, quad_top};
			vec2f_t clipped_max = {quad_right, quad_bottom};

			const f32 ux	= g->uv_x;
			const f32 uy	= g->uv_y;
			const f32 uw	= g->uv_w;
			const f32 uh	= g->uv_h;
			f32		  u0	= ux;
			f32		  u1	= ux + uw;
			f32		  uv_v0 = paint.flip_uv ? uy + uh : uy;
			f32		  uv_v1 = paint.flip_uv ? uy : uy + uh;

			if (cpu_clip && !clip_rect_to_cpu(clipped_min, clipped_max))
			{
				pen.x += g->advance_x * scale + spacing;
				prev = c;
				continue;
			}

			if (cpu_clip)
			{
				const f32 inv_w	 = (quad_right - quad_left) > 0.0f ? 1.0f / (quad_right - quad_left) : 0.0f;
				const f32 inv_h	 = (quad_bottom - quad_top) > 0.0f ? 1.0f / (quad_bottom - quad_top) : 0.0f;
				const f32 tx0	 = (clipped_min.x - quad_left) * inv_w;
				const f32 tx1	 = (clipped_max.x - quad_left) * inv_w;
				const f32 ty0	 = (clipped_min.y - quad_top) * inv_h;
				const f32 ty1	 = (clipped_max.y - quad_top) * inv_h;
				const f32 old_u0 = u0;
				const f32 old_v0 = uv_v0;
				u0				 = math::lerp(old_u0, u1, tx0);
				u1				 = math::lerp(old_u0, u1, tx1);
				uv_v0			 = math::lerp(old_v0, uv_v1, ty0);
				uv_v1			 = math::lerp(old_v0, uv_v1, ty1);
			}

			vg_vertex_t& v0 = verts[emitted_chars * 4 + 0];
			vg_vertex_t& v1 = verts[emitted_chars * 4 + 1];
			vg_vertex_t& v2 = verts[emitted_chars * 4 + 2];
			vg_vertex_t& v3 = verts[emitted_chars * 4 + 3];

			v0.pos = {clipped_min.x, clipped_min.y};
			v1.pos = {clipped_max.x, clipped_min.y};
			v2.pos = {clipped_max.x, clipped_max.y};
			v3.pos = {clipped_min.x, clipped_max.y};

			v0.color = paint.color;
			v1.color = paint.color;
			v2.color = paint.color;
			v3.color = paint.color;

			v0.uv = {u0, uv_v0};
			v1.uv = {u1, uv_v0};
			v2.uv = {u1, uv_v1};
			v3.uv = {u0, uv_v1};

			const u32 base				   = vtx_base + emitted_chars * 4;
			indices[emitted_chars * 6 + 0] = static_cast<vg_index_t>(base + 0);
			indices[emitted_chars * 6 + 1] = static_cast<vg_index_t>(base + 1);
			indices[emitted_chars * 6 + 2] = static_cast<vg_index_t>(base + 3);
			indices[emitted_chars * 6 + 3] = static_cast<vg_index_t>(base + 1);
			indices[emitted_chars * 6 + 4] = static_cast<vg_index_t>(base + 2);
			indices[emitted_chars * 6 + 5] = static_cast<vg_index_t>(base + 3);

			pen.x += g->advance_x * scale + spacing;
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
				e.hash		= cache_hash;
				e.vtx_start = _text_cache_vertex_count;
				e.vtx_count = vtx_count;
				e.idx_start = _text_cache_index_count;
				e.idx_count = idx_count;

				SFG_MEMCPY(&_text_cache_vertex_buffer[_text_cache_vertex_count], verts, vtx_count * sizeof(vg_vertex_t));
				vg_index_t* cache_idx = &_text_cache_index_buffer[_text_cache_index_count];
				for (u32 i = 0; i < idx_count; ++i)
					cache_idx[i] = static_cast<vg_index_t>(indices[i] - vtx_base);

				_text_cache_vertex_count += vtx_count;
				_text_cache_index_count += idx_count;
				_text_cache.push_back(e);
			}

			for (u32 i = 0; i < vtx_count; ++i)
			{
				verts[i].pos.x += draw_pos.x;
				verts[i].pos.y += draw_pos.y;
			}
		}
		else if (emitted_chars != char_count)
		{
			db->vertex_count = vtx_base + emitted_chars * 4;
			db->index_count	 = idx_base + emitted_chars * 6;
		}
	}

	void vg_canvas_t::clear_text_cache()
	{
		_text_cache.resize(0);
		_text_cache_vertex_count = 0;
		_text_cache_index_count	 = 0;
	}
}
