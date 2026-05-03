// Copyright (c) 2025 Inan Evin

#include "atlas.hpp"
#include "font.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>

namespace sfg
{
	atlas_t::~atlas_t()
	{
		SFG_ASSERT(_data == nullptr);
	}

	void atlas_t::init(u32 width, u32 height, bool is_lcd)
	{
		SFG_ASSERT(width > 0 && height > 0);
		_width	   = width;
		_height	   = height;
		_is_lcd	   = is_lcd;
		_data_size = width * height * (is_lcd ? 3u : 1u);
		_data	   = static_cast<u8*>(SFG_MALLOC(_data_size));
		SFG_MEMTRACE_ALLOC(_data, _data_size);
		SFG_MEMSET(_data, 0, _data_size);
		_vertical_pos = 0;
		_font_count	  = 0;
		_dirty		  = true;
	}

	void atlas_t::uninit()
	{
		if (_data)
		{
			SFG_MEMTRACE_DEALLOC(_data);
			SFG_FREE(_data);
		}
		_data		  = nullptr;
		_data_size	  = 0;
		_vertical_pos = 0;
		_font_count	  = 0;
		_dirty		  = false;
	}

	bool atlas_t::add_font(font_data_t* font)
	{
		SFG_ASSERT(font != nullptr);

		i32		  total_width = 0;
		i32		  max_height  = 0;
		const i32 x_padding	  = 2;
		for (u32 i = 0; i < 128; ++i)
		{
			const font_glyph_t& g = font->glyph_info[i];
			if (g.width >= 1)
				total_width += g.width + x_padding;
			max_height = math::max(max_height, g.height);
		}

		const i32 atlas_w		  = static_cast<i32>(_width);
		const i32 required_rows	  = atlas_w > 0 ? static_cast<i32>(math::ceil(static_cast<f32>(total_width) / static_cast<f32>(atlas_w))) : 0;
		const u32 required_height = static_cast<u32>(required_rows * max_height);

		if (_vertical_pos + required_height > _height)
			return false;

		const u32 slot_top = _vertical_pos;

		i32 pen_x = 0;
		i32 pen_y = 0;

		for (u32 i = 0; i < 128; ++i)
		{
			font_glyph_t& g = font->glyph_info[i];
			if (g.width <= 0 || g.height <= 0 || g.pixel_size == 0)
				continue;

			if (pen_x + g.width > atlas_w)
			{
				pen_x = 0;
				pen_y += max_height;
			}

			SFG_ASSERT(pen_y + g.height <= static_cast<i32>(required_height));

			g.atlas_x = pen_x;
			g.atlas_y = static_cast<i32>(slot_top) + pen_y;
			g.uv_x	  = static_cast<f32>(g.atlas_x) / static_cast<f32>(_width);
			g.uv_y	  = static_cast<f32>(g.atlas_y) / static_cast<f32>(_height);
			g.uv_w	  = static_cast<f32>(g.width) / static_cast<f32>(_width);
			g.uv_h	  = static_cast<f32>(g.height) / static_cast<f32>(_height);

			// TODO: copy this glyph's pixels from the font's pixel blob into _data at (atlas_x, atlas_y).
			// Source bytes live at font->pixels chunk + g.pixel_offset, but the atlas does not own a
			// chunk_allocator; resolution will be threaded through when create_internals is wired up.

			pen_x += g.width + x_padding;
		}

		_vertical_pos += required_height;
		_font_count++;
		_dirty = true;
		return true;
	}

	void atlas_t::remove_font(font_data_t*)
	{
		SFG_ASSERT(_font_count > 0);
		_font_count--;
	}
}
