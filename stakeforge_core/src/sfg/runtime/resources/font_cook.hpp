// Copyright (c) 2025 Inan Evin
#pragma once

#include "font.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/data/span.hpp>

namespace sfg
{
	class ostream_t;

	struct font_cook_glyph_t
	{
		vector_t<u8> pixels;
		i32			 width			   = 0;
		i32			 height			   = 0;
		i32			 advance_x		   = 0;
		i32			 left_bearing	   = 0;
		f32			 x_offset		   = 0.0f;
		f32			 y_offset		   = 0.0f;
		i32			 kern_advance[128] = {0};
	};

	struct font_cook_t
	{
		font_cook_glyph_t glyphs[128] = {};
		i32				  ascent	  = 0;
		i32				  descent	  = 0;
		i32				  line_gap	  = 0;
		u32				  size		  = 0;
		f32				  scale		  = 0.0f;
		font_kind_e		  kind		  = font_kind_e::bitmap;
	};

	bool font_cook_from_ttf(span_t<const u8> ttf, const font_config_t& cfg, font_cook_t& out);
	bool font_cook_serialize(const font_cook_t& src, ostream_t& stream);
}
