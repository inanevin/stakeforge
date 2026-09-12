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

#pragma once

#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
	struct font_runtime_t;
}

namespace sfg::ui
{

	using widget_id_t = u16;
#define NULL_WIDGET 0xFFFFu

	enum class glyph_raster_mode_e : u8
	{
		lcd,
		grayscale,
		sdf,
	};

	enum class ui_resource_type_e : u8
	{
		none,
		gpu_index,
		gpu_index_fof,
		texture,
		test,
		sprite,
	};

	enum class clip_mode_e : u8
	{
		none,
		cpu_rect,
		scissor_rect,
	};

	struct ui_resource_ref_t
	{
		gpu_index_t		   gpu_indices[BACK_BUFFER_COUNT] = {NULL_GPU_INDEX, NULL_GPU_INDEX, NULL_GPU_INDEX};
		resource_handle_t  handle						  = NULL_RESOURCE_HANDLE;
		ui_resource_type_e type							  = ui_resource_type_e::none;
	};

	struct ui_render_state_t
	{
		resource_handle_t pipeline	   = NULL_RESOURCE_HANDLE;
		ui_resource_ref_t constants[4] = {};
	};

	struct ui_resolved_resource_ref_t
	{
		gpu_index_t				 gpu_indices[BACK_BUFFER_COUNT] = {NULL_GPU_INDEX, NULL_GPU_INDEX, NULL_GPU_INDEX};
		render_resource_handle_t texture						= {};
		ui_resource_type_e		 type							= ui_resource_type_e::none;
	};

	struct ui_resolved_state_t
	{
		render_resource_handle_t   pipeline		= {};
		ui_resolved_resource_ref_t constants[4] = {};
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

	struct vg_arc_paint_t
	{
		vec4f_t color		 = {1, 1, 1, 1};
		f32		thickness	 = 1.0f;
		f32		aa_thickness = 0.0f;
		u32		segments	 = 32;
	};

	struct vg_convex_paint_t
	{
		vec4f_t		  fill_color_a = {1, 1, 1, 1};
		vec4f_t		  fill_color_b = {1, 1, 1, 1};
		f32			  aa_thickness = 0.0f;
		vg_gradient_e gradient	   = vg_gradient_e::none;
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

	struct vg_text_run_handle_t
	{
		u32 index	   = UINT32_MAX;
		u32 generation = 0;

		inline bool is_null() const
		{
			return index == UINT32_MAX;
		}
	};

	inline f32 get_valid_scale(f32 scale)
	{
		return scale > 0.0f ? scale : 1.0f;
	}

	inline u32 get_text_raster_px(f32 size_px)
	{
		const i32 px = static_cast<i32>(size_px + 0.5f);
		return px > 0 ? static_cast<u32>(px) : 1;
	}

	inline u32 get_text_paint_raster_px(const vg_text_paint_t& paint)
	{
		return paint.raster_px > 0 ? paint.raster_px : 1;
	}

	inline f32 get_text_paint_draw_scale(const vg_text_paint_t& paint, u32 raster_px)
	{
		return paint.size_px > 0.0f ? paint.size_px / static_cast<f32>(raster_px) : 1.0f;
	}

}
