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
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	struct font_runtime_t;
}

namespace sfg::ui
{
	constexpr u32 INVALID_ID_U32 = 0xFFFFFFFFu;
	constexpr u16 INVALID_ID_U16 = 0xFFFFu;

	using widget_id_t = u16;
#define INVALID_WIDGET 0xFFFFu

	enum class glyph_raster_mode_e : u8
	{
		lcd,
		sdf,
	};

	enum class ui_resource_type_e : u8
	{
		none,
		texture,
	};

	struct ui_resource_ref_t
	{
		resource_handle_t  handle = NULL_RESOURCE_HANDLE;
		ui_resource_type_e type	  = ui_resource_type_e::none;
	};

	struct ui_render_state_t
	{
		resource_handle_t pipeline	   = NULL_RESOURCE_HANDLE;
		ui_resource_ref_t constants[4] = {};
	};

	struct ui_resolved_state_t
	{
		gfx_shader_handle pipeline	   = {};
		gpu_index_t		  constants[4] = {};
	};

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

	struct vg_text_style_t
	{
		resource_handle_t	font		= NULL_RESOURCE_HANDLE;
		vec4f_t				color		= {1, 1, 1, 1};
		f32					point_size	= 13.0f;
		u8					spacing		= 0;
		glyph_raster_mode_e raster_mode = glyph_raster_mode_e::lcd;
		bool				flip_uv		= false;
	};

	struct vg_text_paint_t
	{
		const font_runtime_t* font		  = nullptr;
		vec4f_t				  color		  = {1, 1, 1, 1};
		f32					  size_px	  = 13.0f;
		u32					  raster_px	  = 13;
		f32					  spacing	  = 0.0f;
		glyph_raster_mode_e	  raster_mode = glyph_raster_mode_e::lcd;
		bool				  flip_uv	  = false;
	};

}
