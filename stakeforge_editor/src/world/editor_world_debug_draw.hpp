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

#include <sfg/data/span.hpp>
#include <sfg/data/unique.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg::ui
{
	class vg_canvas_t;
}

namespace sfg
{
	class color_t;
	class mat4x3_t;
	struct aabb_t;

	enum class editor_debug_depth_e : u8
	{
		depth_tested,
		always_visible,
	};

	enum class editor_debug_text_alignment_e : u8
	{
		top_left,
		top_center,
		top_right,
		center_left,
		center,
		center_right,
		bottom_left,
		bottom_center,
		bottom_right,
	};

	struct editor_world_debug_line_vertex_t
	{
		vec4f_t color				= vec4f_t::zero;
		vec3f_t position			= vec3f_t::zero;
		vec3f_t other_position		= vec3f_t::zero;
		f32		corner				= 0.0f;
		f32		signed_thickness_px = 1.0f;
	};

	struct editor_world_debug_line_snapshot_t
	{
		vector_t<editor_world_debug_line_vertex_t> vertices;
		vector_t<primitive_index>				   indices;
	};

	struct editor_world_debug_text_vertex_t
	{
		vec4f_t color  = vec4f_t::zero;
		vec3f_t anchor = vec3f_t::zero;
		vec2f_t offset = vec2f_t::zero;
		vec2f_t uv	   = vec2f_t::zero;
		f32		mode   = 0.0f;
	};

	static_assert(sizeof(editor_world_debug_text_vertex_t) == sizeof(f32) * 12);

	struct editor_world_debug_text_snapshot_t
	{
		vector_t<editor_world_debug_text_vertex_t> vertices;
		vector_t<primitive_index>				   indices;
	};

	class editor_world_debug_draw_t final
	{
	public:
		static inline constexpr u32 MAX_LINE_COUNT		   = 8192;
		static inline constexpr u32 MAX_VERTEX_COUNT	   = MAX_LINE_COUNT * 4;
		static inline constexpr u32 MAX_INDEX_COUNT		   = MAX_LINE_COUNT * 6;
		static inline constexpr u32 MAX_TEXT_COMMAND_COUNT = 256;
		static inline constexpr u32 MAX_TEXT_BYTE_COUNT	   = 32768;
		static inline constexpr u32 MAX_TEXT_VERTEX_COUNT  = 16384;
		static inline constexpr u32 MAX_TEXT_INDEX_COUNT   = 24576;

		editor_world_debug_draw_t();
		~editor_world_debug_draw_t();
		editor_world_debug_draw_t(const editor_world_debug_draw_t&)			   = delete;
		editor_world_debug_draw_t& operator=(const editor_world_debug_draw_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void begin_frame();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void draw_line(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested);
		void draw_polyline(span_t<const vec3f_t> points, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested, bool closed = false);
		void draw_aabb(const aabb_t& bounds, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested);
		void draw_box(const mat4x3_t& transform, const vec3f_t& half_extents, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested);
		void draw_circle(const vec3f_t& center, f32 radius, const vec3f_t& normal, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested, u32 segments = 32);
		void draw_sphere(const vec3f_t& center, f32 radius, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested, u32 segments = 32);
		void draw_capsule(const vec3f_t& center, f32 radius, f32 half_height, const vec3f_t& direction, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested, u32 segments = 32);
		void draw_frustum(const vec3f_t& origin, const vec3f_t& direction, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px = 2.0f, editor_debug_depth_e depth = editor_debug_depth_e::depth_tested);
		void draw_text_2d(const vec2f_t& position, const char* text, const color_t& color, f32 size_px = 14.0f, editor_debug_text_alignment_e alignment = editor_debug_text_alignment_e::top_left, resource_handle_t font = NULL_RESOURCE_HANDLE);
		void draw_text_3d(const vec3f_t&				position,
						  const char*					text,
						  const color_t&				color,
						  f32							size_px		  = 14.0f,
						  editor_debug_depth_e			depth		  = editor_debug_depth_e::always_visible,
						  editor_debug_text_alignment_e alignment	  = editor_debug_text_alignment_e::center,
						  const vec2f_t&				screen_offset = vec2f_t::zero,
						  resource_handle_t				font		  = NULL_RESOURCE_HANDLE);
		void write_snapshot(editor_world_debug_line_snapshot_t& snapshot) const;
		void write_text_snapshot(editor_world_debug_text_snapshot_t& snapshot);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline u32 get_dropped_line_count() const
		{
			return _dropped_line_count;
		}

		inline u32 get_dropped_text_count() const
		{
			return _dropped_text_count;
		}

	private:
		struct text_command_t
		{
			resource_handle_t			  font			= NULL_RESOURCE_HANDLE;
			vec4f_t						  color			= vec4f_t::zero;
			vec3f_t						  anchor		= vec3f_t::zero;
			vec2f_t						  screen_offset = vec2f_t::zero;
			f32							  size_px		= 14.0f;
			u32							  text_offset	= 0;
			u32							  text_length	= 0;
			editor_debug_text_alignment_e alignment		= editor_debug_text_alignment_e::top_left;
			editor_debug_depth_e		  depth			= editor_debug_depth_e::always_visible;
			bool						  is_screen		= false;
		};

		vector_t<editor_world_debug_line_vertex_t> _vertices;
		vector_t<primitive_index>				   _indices;
		vector_t<text_command_t>				   _text_commands;
		vector_t<char>							   _text_bytes;
		unique_t<ui::vg_canvas_t>				   _text_canvas;
		u32										   _dropped_line_count = 0;
		u32										   _dropped_text_count = 0;
	};
}
