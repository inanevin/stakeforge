// Copyright (c) 2025 Inan Evin

#include "font_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <sfg/vendor/stb/stb_truetype.h>

namespace sfg
{
	namespace
	{
		struct glyph_data_t
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
	}

	bool font_cooker::cook_from_file(const font_cook_config_t& cfg, const char* full_path, ostream_t& stream)
	{
		if (cfg.range_start >= cfg.range_end || cfg.range_end > 128)
		{
			SFG_ERR("invalid glyph range");
			return false;
		}

		char*  ttf_data = nullptr;
		size_t ttf_size = 0;
		file_system_t::read_file(full_path, ttf_data, ttf_size);
		if (ttf_data == nullptr || ttf_size == 0)
		{
			SFG_ERR("failed to read font file {0}", full_path);
			return false;
		}

		const u8* ttf_bytes = reinterpret_cast<const u8*>(ttf_data);

		stbtt_fontinfo stb_font;
		stbtt_InitFont(&stb_font, ttf_bytes, stbtt_GetFontOffsetForIndex(ttf_bytes, 0));

		const f32 scale	 = stbtt_ScaleForMappingEmToPixels(&stb_font, static_cast<f32>(cfg.size));
		i32		  ascent = 0, descent = 0, line_gap = 0;
		stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &line_gap);

		vector_t<glyph_data_t> glyphs;
		glyphs.resize(128);

		for (u32 i = cfg.range_start; i < cfg.range_end; ++i)
		{
			glyph_data_t& g = glyphs[i];

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

		delete[] ttf_data;

		u32 total_pixels = 0;
		u32 offsets[128] = {0};
		for (u32 i = 0; i < 128; ++i)
		{
			offsets[i] = total_pixels;
			total_pixels += static_cast<u32>(glyphs[i].pixels.size());
		}

		const size_t	  header_pos = stream.get_size();
		resource_header_t header	 = {
				.magic		  = font_loader_t::WIRE_MAGIC,
				.version	  = font_loader_t::WIRE_VERSION,
				.payload_size = total_pixels,
				.source_ticks = {file_system_t::get_last_modified_ticks(full_path)},
		};
		header.serialize(stream);

		stream << ascent << descent << line_gap;
		stream << cfg.size << scale << cfg.kind;

		for (u32 i = 0; i < 128; ++i)
		{
			const glyph_data_t& g	 = glyphs[i];
			const u32			size = static_cast<u32>(g.pixels.size());
			stream << offsets[i] << size;
			stream << g.width << g.height << g.advance_x << g.left_bearing;
			stream << g.x_offset << g.y_offset;
			for (i32 k = 0; k < 128; ++k)
				stream << g.kern_advance[k];
		}

		header.payload_offset = static_cast<u32>(stream.get_size() - header_pos);
		header.patch_payload_offset(stream, header_pos);

		for (u32 i = 0; i < 128; ++i)
		{
			const glyph_data_t& g = glyphs[i];
			if (!g.pixels.empty())
				stream.write_raw(g.pixels.data(), g.pixels.size());
		}

		return true;
	}

	void from_json(const nlohmann::json& j, font_kind_e& k)
	{
		const string_t s = j.get<string_t>();

		if (s == "sdf")
			k = font_kind_e::sdf;
		else if (s == "lcd")
			k = font_kind_e::lcd;
		else
			k = font_kind_e::bitmap;
	}

	void from_json(const nlohmann::json& j, font_cook_config_t& c)
	{
		c.size		   = j.value<u32>("size", 16);
		c.range_start  = j.value<u32>("range_start", 32);
		c.range_end	   = j.value<u32>("range_end", 128);
		c.sdf_padding  = j.value<i32>("sdf_padding", 3);
		c.sdf_edge	   = j.value<i32>("sdf_edge", 128);
		c.sdf_distance = j.value<f32>("sdf_distance", 32.0f);
		c.kind		   = j.value<font_kind_e>("kind", font_kind_e::bitmap);
	}
}
