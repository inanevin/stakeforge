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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	class resource_manager_t;
}

namespace sfg::ui
{

	struct vg_draw_buffer_t
	{
		vg_vertex_t*		vertex_start	= nullptr;
		vg_index_t*			index_start		= nullptr;
		ui_render_state_t	state			= {};
		ui_resolved_state_t resolved		= {};
		vec4f_t				clip			= {0, 0, 0, 0};
		u32					draw_order		= 0;
		u32					vertex_count	= 0;
		u32					index_count		= 0;
		u32					vertex_capacity = 0;
		u32					index_capacity	= 0;
	};

	struct vg_draw_buffer_final_t
	{
		ui_resolved_state_t resolved	  = {};
		vec4f_t				clip		  = {0, 0, 0, 0};
		u32					draw_order	  = 0;
		u32					vertex_count  = 0;
		u32					index_count	  = 0;
		u32					vertex_offset = 0;
		u32					index_offset  = 0;
	};

	struct vg_draw_snapshot_t
	{
		const vg_draw_buffer_final_t* draw_buffers		= nullptr;
		const vg_vertex_t*			  vertices			= nullptr;
		const vg_index_t*			  indices			= nullptr;
		u32							  draw_buffer_count = 0;
		u32							  vertex_count		= 0;
		u32							  index_count		= 0;
	};

	struct vg_canvas_config_t
	{
		u64 vertex_buffer_bytes		= 1u << 21; // 2 MB
		u64 index_buffer_bytes		= 1u << 21; // 2 MB
		u32 buffer_count			= 16;
		u32 text_cache_vertex_bytes = 1u << 22;
		u32 text_cache_index_bytes	= 1u << 22;
		u32 clip_stack_capacity		= 64;
	};

	class vg_canvas_t
	{
	public:
		vg_canvas_t()							   = default;
		vg_canvas_t(const vg_canvas_t&)			   = delete;
		vg_canvas_t& operator=(const vg_canvas_t&) = delete;
		~vg_canvas_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const vg_canvas_config_t& cfg);
		void uninit();
		void frame_begin(const vec4f_t& screen_clip);
		void frame_end();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void		   push_clip(const vec4f_t& rect, clip_mode_e mode);
		void		   pop_clip(clip_mode_e mode);
		vec4f_t		   current_scissor_clip() const;
		vec4f_t		   current_cpu_clip() const;
		void		   clear_text_cache();
		static vec2f_t measure_text(const char* text, size_t len, const vg_text_paint_t& paint);

		// -----------------------------------------------------------------------------
		// pipelines
		// -----------------------------------------------------------------------------

		void resolve();

		// -----------------------------------------------------------------------------
		// paint
		// -----------------------------------------------------------------------------

		void add_rect(const vec2f_t& min, const vec2f_t& max, const vg_rect_paint_t& paint, const ui_render_state_t& state, u32 draw_order = 0);
		void add_line(const vec2f_t& p0, const vec2f_t& p1, const vg_line_paint_t& paint, const ui_render_state_t& state, u32 draw_order = 0);
		void add_circle(const vec2f_t& center, f32 radius, const vg_circle_paint_t& paint, const ui_render_state_t& state, u32 draw_order = 0);
		void add_convex(span_t<const vec2f_t> path, const vg_convex_paint_t& paint, const ui_render_state_t& state, u32 draw_order = 0);
		void add_text(const char* text, size_t len, const vec2f_t& pos, const vg_text_paint_t& paint, const ui_render_state_t& state, u32 draw_order = 0, bool use_cache = true);

		// -----------------------------------------------------------------------------
		// emit
		// -----------------------------------------------------------------------------

		vg_draw_buffer_t* get_draw_buffer(u32 draw_order, const ui_render_state_t& state);

		static void emit_path_solid(vg_draw_buffer_t* db, span_t<const vec2f_t> path, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max);
		static void emit_path_grad(vg_draw_buffer_t* db, span_t<const vec2f_t> path, const vec4f_t& color_a, const vec4f_t& color_b, vg_gradient_e dir, const vec2f_t& min, const vec2f_t& max);
		static void emit_central_solid(vg_draw_buffer_t* db, const vec4f_t& color, const vec2f_t& min, const vec2f_t& max);
		static void emit_central_grad(vg_draw_buffer_t* db, const vec4f_t& color_a, const vec4f_t& color_b, const vec2f_t& min, const vec2f_t& max);
		static void emit_path_alpha(vg_draw_buffer_t* db, span_t<const vec2f_t> path, u32 source_vtx_base, f32 alpha, const vec2f_t& min, const vec2f_t& max);
		static void emit_quad_indices(vg_draw_buffer_t* db, u32 base);
		static void emit_fan_indices(vg_draw_buffer_t* db, u32 ring_base, u32 center_idx, u32 ring_size);
		static void emit_strip_indices(vg_draw_buffer_t* db, u32 outer_base, u32 inner_base, u32 ring_size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const vector_t<vg_draw_buffer_t>& get_draw_buffers() const
		{
			return _draw_buffers;
		}

	private:
		bool	has_cpu_clip() const;
		vec4f_t intersect_clip(const vec4f_t& a, const vec4f_t& b) const;
		bool	clip_rect_to_cpu(vec2f_t& min, vec2f_t& max) const;
		bool	clip_line_to_cpu(vec2f_t& p0, vec2f_t& p1, f32 thickness) const;

	private:
		struct text_cache_entry_t
		{
			u64 hash	  = 0;
			u32 vtx_start = 0;
			u32 vtx_count = 0;
			u32 idx_start = 0;
			u32 idx_count = 0;
		};

		struct clip_entry_t
		{
			vec4f_t rect;
		};

		vector_t<vg_draw_buffer_t>	 _draw_buffers;
		vector_t<clip_entry_t>		 _scissor_clip_stack;
		vector_t<clip_entry_t>		 _cpu_clip_stack;
		vector_t<text_cache_entry_t> _text_cache;
		vector_t<vec2f_t>			 _path0;
		vector_t<vec2f_t>			 _path1;
		vector_t<vec2f_t>			 _path2;
		vg_vertex_t*				 _vertex_pool				 = nullptr;
		vg_index_t*					 _index_pool				 = nullptr;
		vg_vertex_t*				 _text_cache_vertex_buffer	 = nullptr;
		vg_index_t*					 _text_cache_index_buffer	 = nullptr;
		u32							 _vertex_capacity_per_buffer = 0;
		u32							 _index_capacity_per_buffer	 = 0;
		u32							 _buffer_count				 = 0;
		u32							 _buffer_counter			 = 0;
		u32							 _text_cache_vertex_capacity = 0;
		u32							 _text_cache_vertex_count	 = 0;
		u32							 _text_cache_index_capacity	 = 0;
		u32							 _text_cache_index_count	 = 0;
	};
}
