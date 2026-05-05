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

	class font_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = 0x53464E54;
		static constexpr u32 WIRE_VERSION = 3;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct font_runtime_glyph_t
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

	struct font_runtime_t
	{
		i32					 ascent			 = 0;
		i32					 descent		 = 0;
		i32					 line_gap		 = 0;
		u32					 size			 = 0;
		f32					 scale			 = 0.0f;
		font_kind_e			 kind			 = font_kind_e::bitmap;
		font_runtime_glyph_t glyph_info[128] = {};
	};

	struct font_internals_t
	{
		u32 reserved = 0;
	};

	extern const resource_type_desc_t font_resource_desc;
}
