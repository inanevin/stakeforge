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
#include <sfg/data/vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/ui/ui_common.hpp>

namespace sfg::ui
{
	struct vg_vertex_t
	{
		vec2f_t pos;
		vec2f_t uv;
		vec4f_t color;
	};

	using vg_index_t = u16;

	enum class vg_gradient_e : u8
	{
		none,
		horizontal,
		vertical,
	};

	struct vg_rect_paint_t
	{
		vec4f_t		  fill_color_a		= {1, 1, 1, 1};
		vec4f_t		  fill_color_b		= {1, 1, 1, 1};
		vec4f_t		  outline_color		= {0, 0, 0, 1};
		f32			  rounding			= 0.0f;
		f32			  outline_thickness = 0.0f;
		f32			  aa_thickness		= 0.0f;
		u16			  rounding_segs		= 0;
		vg_gradient_e gradient			= vg_gradient_e::none;
		bool		  filled			= true;
	};

	struct vg_line_paint_t
	{
		vec4f_t color		 = {1, 1, 1, 1};
		f32		thickness	 = 1.0f;
		f32		aa_thickness = 0.0f;
	};

	struct vg_circle_paint_t
	{
		vec4f_t color		 = {1, 1, 1, 1};
		f32		thickness	 = 1.0f; // used when filled = false
		f32		aa_thickness = 0.0f;
		u32		segments	 = 32;
		bool	filled		 = true;
	};

	struct vg_text_paint_t
	{
		font_runtime_t* font	= nullptr;
		vec4f_t			color	= {1, 1, 1, 1};
		f32				scale	= 1.0f;
		u8				spacing = 0;
		bool			flip_uv = false;
	};

	struct vg_draw_buffer_t
	{
		vg_vertex_t* vertex_start	 = nullptr;
		vg_index_t*	 index_start	 = nullptr;
		void*		 user_data		 = nullptr;
		vec4f_t		 clip			 = {0, 0, 0, 0};
		u32			 atlas_id		 = INVALID_ID_U32;
		u32			 font_id		 = INVALID_ID_U32;
		u32			 draw_order		 = 0;
		u32			 vertex_count	 = 0;
		u32			 index_count	 = 0;
		u32			 vertex_capacity = 0;
		u32			 index_capacity	 = 0;
		font_kind_e	 font_kind		 = font_kind_e::bitmap;
	};

	struct vg_canvas_config_t
	{
		u64 vertex_buffer_bytes		= 1u << 20; // 1 MB
		u64 index_buffer_bytes		= 1u << 20; // 1 MB
		u32 buffer_count			= 64;
		u32 text_cache_vertex_bytes = 1u << 20;
		u32 text_cache_index_bytes	= 1u << 20;
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

		void		   push_clip(const vec4f_t& rect);
		void		   pop_clip();
		vec4f_t		   current_clip() const;
		void		   clear_text_cache();
		static vec2f_t measure_text(const char* text, size_t len, const vg_text_paint_t& paint);

		// -----------------------------------------------------------------------------
		// paint
		// -----------------------------------------------------------------------------

		void add_rect(const vec2f_t& min, const vec2f_t& max, const vg_rect_paint_t& paint, u32 draw_order = 0, void* user_data = nullptr);
		void add_line(const vec2f_t& p0, const vec2f_t& p1, const vg_line_paint_t& paint, u32 draw_order = 0, void* user_data = nullptr);
		void add_circle(const vec2f_t& center, f32 radius, const vg_circle_paint_t& paint, u32 draw_order = 0, void* user_data = nullptr);
		void add_text(const char* text, size_t len, const vec2f_t& pos, const vg_text_paint_t& paint, u32 draw_order = 0, void* user_data = nullptr, bool use_cache = true);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const vector_t<vg_draw_buffer_t>& get_draw_buffers() const
		{
			return _draw_buffers;
		}

	private:
		vg_draw_buffer_t* get_draw_buffer(u32 draw_order, void* user_data, font_runtime_t* font);
		vec4f_t			  intersect_clip(const vec4f_t& a, const vec4f_t& b) const;

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
		vector_t<clip_entry_t>		 _clip_stack;
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
