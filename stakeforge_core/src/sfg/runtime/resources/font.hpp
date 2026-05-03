// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"

namespace sfg
{
	enum class font_kind_e : u8
	{
		bitmap,
		sdf,
		lcd,
	};

	struct font_glyph_t
	{
		u32 pixel_offset	  = 0;
		u32 pixel_size		  = 0;
		i32 width			  = 0;
		i32 height			  = 0;
		i32 advance_x		  = 0;
		i32 left_bearing	  = 0;
		f32 x_offset		  = 0.0f;
		f32 y_offset		  = 0.0f;
		i32 kern_advance[128] = {0};

		// runtime-only atlas placement (populated when this font is added to an atlas, not part of the cooked wire format)
		i32 atlas_x = 0;
		i32 atlas_y = 0;
		f32 uv_x	= 0.0f;
		f32 uv_y	= 0.0f;
		f32 uv_w	= 0.0f;
		f32 uv_h	= 0.0f;
	};

	struct font_data_t
	{
		chunk_handle32_t pixels			 = {};
		u32				 pixels_size	 = 0;
		i32				 ascent			 = 0;
		i32				 descent		 = 0;
		i32				 line_gap		 = 0;
		u32				 size			 = 0;
		f32				 scale			 = 0.0f;
		font_kind_e		 kind			 = font_kind_e::bitmap;
		font_glyph_t	 glyph_info[128] = {};
	};

	struct font_internals_t
	{
		u32 reserved = 0;
	};

	struct font_config_t
	{
		u32			size		 = 16;
		u32			range_start	 = 32;
		u32			range_end	 = 128;
		i32			sdf_padding	 = 3;
		i32			sdf_edge	 = 128;
		f32			sdf_distance = 32.0f;
		font_kind_e kind		 = font_kind_e::bitmap;
	};

	extern bool font_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx);
	extern bool font_create_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void font_destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	extern void font_unload(resource_entry_t& entry, resource_context_t& ctx);
	extern void font_unload_cpu(resource_entry_t& entry, resource_context_t& ctx);

	extern const resource_type_desc_t font_resource_desc;

	inline constexpr u32 font_wire_magic   = 0x53464E54;
	inline constexpr u32 font_wire_version = 2;
}
