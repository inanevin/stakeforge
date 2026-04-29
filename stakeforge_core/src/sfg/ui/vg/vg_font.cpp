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

#include "vg_font.hpp"
#include "vg_atlas.hpp"
#include "io/assert.hpp"
#include "io/log.hpp"
#include "math/math.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb/stb_truetype.h"

#include <fstream>
#include <cstring>

namespace sfg::ui
{
	vg_font_manager_t::vg_font_manager_t() = default;

	vg_font_manager_t::~vg_font_manager_t()
	{
		SFG_ASSERT(_atlases.empty());
		SFG_ASSERT(_fonts.empty());
	}

	void vg_font_manager_t::init(u32 atlas_width, u32 atlas_height)
	{
		_atlas_width  = atlas_width;
		_atlas_height = atlas_height;
	}

	void vg_font_manager_t::uninit()
	{
		for (unique_t<vg_atlas_t>& atl : _atlases)
			atl->uninit();
		_atlases.resize(0);

		for (unique_t<vg_font_t>& fnt : _fonts)
		{
			for (u32 i = 0; i < 128; ++i)
			{
				if (fnt->glyph_info[i].sdf_data)
					stbtt_FreeSDF(fnt->glyph_info[i].sdf_data, nullptr);
			}
		}
		_fonts.resize(0);
	}

	void vg_font_manager_t::place_into_atlas(vg_font_t* fnt)
	{
		const bool need_lcd = fnt->kind == vg_font_kind_e::lcd;
		for (unique_t<vg_atlas_t>& atl : _atlases)
		{
			if (atl->get_is_lcd() != need_lcd)
				continue;
			if (atl->add_font(fnt))
				return;
		}

		unique_t<vg_atlas_t> atl = make_unique<vg_atlas_t>();
		atl->init(_atlas_width, _atlas_height, need_lcd);
		atl->set_id(static_cast<u32>(_atlases.size()));
		const bool ok = atl->add_font(fnt);
		SFG_ASSERT(ok);
		_atlases.push_back(std::move(atl));
	}

	vg_font_t* vg_font_manager_t::load_font(const u8* data, size_t data_size, const vg_font_config_t& cfg)
	{
		if (cfg.range_start >= cfg.range_end || cfg.range_end > 128)
		{
			SFG_ERR("vg_font_manager: invalid glyph range");
			return nullptr;
		}

		stbtt_fontinfo stb_font;
		stbtt_InitFont(&stb_font, data, stbtt_GetFontOffsetForIndex(data, 0));

		unique_t<vg_font_t> fnt = make_unique<vg_font_t>();
		fnt->_scale				= stbtt_ScaleForMappingEmToPixels(&stb_font, static_cast<f32>(cfg.size));
		fnt->kind				= cfg.kind;
		fnt->size				= cfg.size;
		stbtt_GetFontVMetrics(&stb_font, &fnt->ascent, &fnt->descent, &fnt->line_gap);

		i32		  total_width = 0;
		i32		  max_height  = 0;
		const i32 x_padding	  = 2;

		for (u32 i = cfg.range_start; i < cfg.range_end; ++i)
		{
			vg_glyph_t& g = fnt->glyph_info[i];

			if (cfg.kind == vg_font_kind_e::sdf)
			{
				i32 x_off = 0, y_off = 0;
				g.sdf_data = stbtt_GetCodepointSDF(&stb_font, fnt->_scale, static_cast<i32>(i), cfg.sdf_padding, static_cast<u8>(cfg.sdf_edge), cfg.sdf_distance, &g.width, &g.height, &x_off, &y_off);
				g.x_offset = static_cast<f32>(x_off);
				g.y_offset = static_cast<f32>(y_off);
			}
			else if (cfg.kind == vg_font_kind_e::lcd)
			{
				i32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBoxSubpixel(&stb_font, static_cast<i32>(i), fnt->_scale * 3.0f, fnt->_scale, 1.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
				g.width	   = ix1 - ix0;
				g.height   = iy1 - iy0;
				g.x_offset = static_cast<f32>(ix0);
				g.y_offset = static_cast<f32>(iy0);
			}
			else
			{
				i32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetCodepointBitmapBox(&stb_font, static_cast<i32>(i), fnt->_scale, fnt->_scale, &ix0, &iy0, &ix1, &iy1);
				g.width	   = ix1 - ix0;
				g.height   = iy1 - iy0;
				g.x_offset = static_cast<f32>(ix0);
				g.y_offset = static_cast<f32>(iy0);
			}

			if (g.width >= 1)
				total_width += g.width + x_padding;
			max_height = math::max(max_height, g.height);
			stbtt_GetCodepointHMetrics(&stb_font, static_cast<i32>(i), &g.advance_x, &g.left_bearing);

			for (i32 j = 0; j < 128; ++j)
				g.kern_advance[j] = stbtt_GetCodepointKernAdvance(&stb_font, static_cast<i32>(i), j);
		}

		const i32 required_rows		= static_cast<i32>(math::ceil(static_cast<f32>(total_width) / static_cast<f32>(_atlas_width)));
		fnt->_atlas_required_height = static_cast<u32>(required_rows * max_height);
		place_into_atlas(fnt.get());

		if (fnt->_atlas == nullptr)
		{
			SFG_ERR("vg_font_manager: failed to place font into atlas");
			return nullptr;
		}

		fnt->_font_id	   = static_cast<u32>(_fonts.size());
		vg_font_t* fnt_raw = fnt.get();
		_fonts.push_back(std::move(fnt));

		i32 pen_x = 0;
		i32 pen_y = 0;

		for (u32 i = cfg.range_start; i < cfg.range_end; ++i)
		{
			vg_glyph_t& g = fnt_raw->glyph_info[i];

			if (g.width <= 0 || g.height <= 0)
				continue;

			if (pen_x + g.width > static_cast<i32>(_atlas_width))
			{
				pen_x = 0;
				pen_y += max_height;
			}

			SFG_ASSERT(pen_y + g.height <= static_cast<i32>(fnt_raw->_atlas_required_height));

			g.atlas_x = pen_x;
			g.atlas_y = static_cast<i32>(fnt_raw->_atlas_pos) + pen_y;
			g.uv_x	  = static_cast<f32>(g.atlas_x) / static_cast<f32>(fnt_raw->_atlas->get_width());
			g.uv_y	  = static_cast<f32>(g.atlas_y) / static_cast<f32>(fnt_raw->_atlas->get_height());
			g.uv_w	  = static_cast<f32>(g.width) / static_cast<f32>(fnt_raw->_atlas->get_width());
			g.uv_h	  = static_cast<f32>(g.height) / static_cast<f32>(fnt_raw->_atlas->get_height());

			const u32 pixel_size = (cfg.kind == vg_font_kind_e::lcd) ? 3u : 1u;
			u8*		  dest		 = fnt_raw->_atlas->get_data() + (static_cast<u32>(g.atlas_y) * fnt_raw->_atlas->get_width() * pixel_size) + static_cast<u32>(g.atlas_x) * pixel_size;

			if (cfg.kind == vg_font_kind_e::sdf)
			{
				const u32 stride = fnt_raw->_atlas->get_width();
				for (i32 row = 0; row < g.height; ++row)
					std::memcpy(dest + row * stride, g.sdf_data + row * g.width, g.width);
			}
			else if (cfg.kind == vg_font_kind_e::lcd)
			{
				stbtt_MakeCodepointBitmapSubpixel(&stb_font, dest, g.width, g.height, static_cast<i32>(fnt_raw->_atlas->get_width()) * 3, fnt_raw->_scale * 3.0f, fnt_raw->_scale, 1.0f, 0.0f, static_cast<i32>(i));
			}
			else
			{
				stbtt_MakeCodepointBitmap(&stb_font, dest, g.width, g.height, static_cast<i32>(fnt_raw->_atlas->get_width()), fnt_raw->_scale, fnt_raw->_scale, static_cast<i32>(i));
			}

			pen_x += g.width + x_padding;
		}

		fnt_raw->_atlas->mark_dirty();
		return fnt_raw;
	}

	vg_font_t* vg_font_manager_t::load_font_from_file(const char* path, const vg_font_config_t& cfg)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			SFG_ERR("vg_font_manager: failed to open font file");
			return nullptr;
		}
		const std::streamsize file_size = file.tellg();
		file.seekg(0, std::ios::beg);

		vector_t<u8> ttf_buffer;
		ttf_buffer.resize(static_cast<size_t>(file_size));
		if (!file.read(reinterpret_cast<char*>(ttf_buffer.data()), file_size))
		{
			SFG_ERR("vg_font_manager: failed to read font buffer");
			return nullptr;
		}
		return load_font(ttf_buffer.data(), ttf_buffer.size(), cfg);
	}

	void vg_font_manager_t::unload_font(vg_font_t* fnt)
	{
		SFG_ASSERT(fnt && fnt->_atlas);
		vg_atlas_t* atl = fnt->_atlas;
		atl->remove_font(fnt);

		if (atl->is_empty())
		{
			atl->uninit();
			for (auto it = _atlases.begin(); it != _atlases.end(); ++it)
			{
				if (it->get() == atl)
				{
					_atlases.erase(it);
					break;
				}
			}
			for (size_t i = 0; i < _atlases.size(); ++i)
				_atlases[i]->set_id(static_cast<u32>(i));
		}

		for (u32 i = 0; i < 128; ++i)
		{
			if (fnt->glyph_info[i].sdf_data)
				stbtt_FreeSDF(fnt->glyph_info[i].sdf_data, nullptr);
		}

		for (auto it = _fonts.begin(); it != _fonts.end(); ++it)
		{
			if (it->get() == fnt)
			{
				_fonts.erase(it);
				break;
			}
		}
		for (size_t i = 0; i < _fonts.size(); ++i)
			_fonts[i]->_font_id = static_cast<u32>(i);
	}
}
