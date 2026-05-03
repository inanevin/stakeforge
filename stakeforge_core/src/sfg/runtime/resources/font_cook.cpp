// Copyright (c) 2025 Inan Evin

#include "font_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <sfg/vendor/stb/stb_truetype.h>

namespace sfg
{
	bool font_cook_from_ttf(span_t<const u8> ttf, const font_config_t& cfg, font_cook_t& out)
	{
		if (cfg.range_start >= cfg.range_end || cfg.range_end > 128)
		{
			SFG_ERR("invalid glyph range");
			return false;
		}

		stbtt_fontinfo stb_font;
		stbtt_InitFont(&stb_font, ttf.data, stbtt_GetFontOffsetForIndex(ttf.data, 0));

		out.scale = stbtt_ScaleForMappingEmToPixels(&stb_font, static_cast<f32>(cfg.size));
		out.kind  = cfg.kind;
		out.size  = cfg.size;
		stbtt_GetFontVMetrics(&stb_font, &out.ascent, &out.descent, &out.line_gap);

		const f32 scale = out.scale;

		for (u32 i = cfg.range_start; i < cfg.range_end; ++i)
		{
			font_cook_glyph_t& g = out.glyphs[i];

			if (cfg.kind == font_kind_e::sdf)
			{
				i32 x_off	   = 0;
				i32 y_off	   = 0;
				u8* stb_buffer = stbtt_GetCodepointSDF(&stb_font, scale, static_cast<i32>(i), cfg.sdf_padding, static_cast<u8>(cfg.sdf_edge), cfg.sdf_distance, &g.width, &g.height, &x_off, &y_off);
				g.x_offset	   = static_cast<f32>(x_off);
				g.y_offset	   = static_cast<f32>(y_off);
				if (stb_buffer && g.width > 0 && g.height > 0)
				{
					const u32 bytes = static_cast<u32>(g.width) * static_cast<u32>(g.height);
					g.pixels.resize(bytes);
					SFG_MEMCPY(g.pixels.data(), stb_buffer, bytes);
				}
				if (stb_buffer)
					stbtt_FreeSDF(stb_buffer, nullptr);
			}
			else if (cfg.kind == font_kind_e::lcd)
			{
				i32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBoxSubpixel(&stb_font, static_cast<i32>(i), scale * 3.0f, scale, 1.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
				g.width	   = ix1 - ix0;
				g.height   = iy1 - iy0;
				g.x_offset = static_cast<f32>(ix0);
				g.y_offset = static_cast<f32>(iy0);
				if (g.width > 0 && g.height > 0)
				{
					const u32 bytes = static_cast<u32>(g.width) * static_cast<u32>(g.height) * 3u;
					g.pixels.resize(bytes);
					SFG_MEMSET(g.pixels.data(), 0, bytes);
					stbtt_MakeCodepointBitmapSubpixel(&stb_font, g.pixels.data(), g.width, g.height, g.width * 3, scale * 3.0f, scale, 1.0f, 0.0f, static_cast<i32>(i));
				}
			}
			else
			{
				i32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBox(&stb_font, static_cast<i32>(i), scale, scale, &ix0, &iy0, &ix1, &iy1);
				g.width	   = ix1 - ix0;
				g.height   = iy1 - iy0;
				g.x_offset = static_cast<f32>(ix0);
				g.y_offset = static_cast<f32>(iy0);
				if (g.width > 0 && g.height > 0)
				{
					const u32 bytes = static_cast<u32>(g.width) * static_cast<u32>(g.height);
					g.pixels.resize(bytes);
					SFG_MEMSET(g.pixels.data(), 0, bytes);
					stbtt_MakeCodepointBitmap(&stb_font, g.pixels.data(), g.width, g.height, g.width, scale, scale, static_cast<i32>(i));
				}
			}

			stbtt_GetCodepointHMetrics(&stb_font, static_cast<i32>(i), &g.advance_x, &g.left_bearing);

			for (i32 j = 0; j < 128; ++j)
				g.kern_advance[j] = stbtt_GetCodepointKernAdvance(&stb_font, static_cast<i32>(i), j);
		}

		return true;
	}

	bool font_cook_serialize(const font_cook_t& src, ostream_t& stream)
	{
		u32 total_pixels = 0;
		u32 offsets[128] = {0};
		for (u32 i = 0; i < 128; ++i)
		{
			offsets[i] = total_pixels;
			total_pixels += static_cast<u32>(src.glyphs[i].pixels.size());
		}

		stream << font_wire_magic;
		stream << font_wire_version;
		stream << src.ascent << src.descent << src.line_gap;
		stream << src.size << src.scale << src.kind;
		stream << total_pixels;

		for (u32 i = 0; i < 128; ++i)
		{
			const font_cook_glyph_t& g	  = src.glyphs[i];
			const u32				 size = static_cast<u32>(g.pixels.size());
			stream << offsets[i] << size;
			stream << g.width << g.height << g.advance_x << g.left_bearing;
			stream << g.x_offset << g.y_offset;
			for (i32 k = 0; k < 128; ++k)
				stream << g.kern_advance[k];
		}

		for (u32 i = 0; i < 128; ++i)
		{
			const font_cook_glyph_t& g = src.glyphs[i];
			if (!g.pixels.empty())
				stream.write_raw(g.pixels.data(), g.pixels.size());
		}

		return true;
	}
}
