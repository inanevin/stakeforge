// Copyright (c) 2025 Inan Evin

#include "glyph_atlas.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/vendor/stb/stb_truetype.h>

namespace sfg::ui
{
	namespace
	{
		constexpr u32 ATLAS_BPP			   = 4;
		constexpr i32 SHELF_PADDING_X	   = 1;
		constexpr i32 SHELF_PADDING_Y	   = 1;
		constexpr u32 PITCH_ALIGNMENT	   = 256;
		constexpr u32 PLACEMENT_ALIGNMENT  = 512;
		constexpr i32 GLYPH_UPLOAD_PADDING = 1;
		constexpr i32 LCD_FILTER_RADIUS	   = 2;
		constexpr i32 LCD_FILTER_WEIGHT_0  = 8;
		constexpr i32 LCD_FILTER_WEIGHT_1  = 77;
		constexpr i32 LCD_FILTER_WEIGHT_2  = 86;
		constexpr i32 SDF_PADDING		   = 8;
		constexpr u8  SDF_ONEDGE_VALUE	   = 128;
		constexpr f32 SDF_DIST_SCALE	   = 16.0f;

		inline u64 make_glyph_key(u64 face_id, u32 codepoint, u32 px_size, glyph_raster_mode_e mode)
		{
			return (face_id ^ (static_cast<u64>(codepoint) * 0x9E3779B97F4A7C15ull) ^ (static_cast<u64>(px_size) * 0xBF58476D1CE4E5B9ull) ^ (static_cast<u64>(mode) * 0x94D049BB133111EBull));
		}

		inline u64 make_size_key(u64 face_id, u32 px_size)
		{
			return face_id ^ (static_cast<u64>(px_size) * 0x94D049BB133111EBull);
		}

		inline u32 align_up(u32 v, u32 a)
		{
			return (v + a - 1u) & ~(a - 1u);
		}

		inline u8 filter_lcd_sample(const u8* row, i32 width, i32 x)
		{
			const i32 x0 = x - 2;
			const i32 x1 = x - 1;
			const i32 x3 = x + 1;
			const i32 x4 = x + 2;
			i32		  v	 = row[x] * LCD_FILTER_WEIGHT_2;
			if (x0 >= 0)
				v += row[x0] * LCD_FILTER_WEIGHT_0;
			if (x1 >= 0)
				v += row[x1] * LCD_FILTER_WEIGHT_1;
			if (x3 < width)
				v += row[x3] * LCD_FILTER_WEIGHT_1;
			if (x4 < width)
				v += row[x4] * LCD_FILTER_WEIGHT_0;
			return static_cast<u8>((v + 128) >> 8);
		}
	}

	glyph_atlas_t::~glyph_atlas_t() = default;

	void glyph_atlas_t::init(const glyph_atlas_config_t& cfg)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(_texture.is_null());
		SFG_ASSERT(cfg.width > 0 && cfg.height > 0);

		gfx_backend& backend = gfx_backend::get();

		_width	= cfg.width;
		_height = cfg.height;
		_frame	= 0;

		texture_desc_t tdesc = {};
		tdesc.texture_format = format_e::r8g8b8a8_unorm;
		tdesc.size			 = {static_cast<u16>(_width), static_cast<u16>(_height)};
		tdesc.flags			 = texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		tdesc.mip_levels	 = 1;
		tdesc.array_length	 = 1;
		tdesc.samples		 = 1;
		tdesc.set_name("ui_glyph_atlas");
		_texture   = backend.create_texture(tdesc);
		_gpu_index = backend.get_texture_gpu_index(_texture, 0);

		const u32 staging_bytes = align_up(cfg.staging_bytes, PLACEMENT_ALIGNMENT);
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			resource_desc_t sdesc = {};
			sdesc.size			  = staging_bytes;
			sdesc.flags			  = resource_flags::rf_cpu_visible;
			sdesc.set_name("ui_glyph_staging");
			_staging[i].buffer	 = backend.create_resource(sdesc);
			_staging[i].capacity = staging_bytes;
			backend.map_resource(_staging[i].buffer, _staging[i].mapped);
		}

		_entries.reserve(1024);
		_metrics.reserve(64);
		_shelves.reserve(64);
		_pending.reserve(256);
		_transitioned = false;
	}

	void glyph_atlas_t::uninit()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		gfx_backend& backend = gfx_backend::get();

		for (pending_upload_t& p : _pending)
		{
			SFG_FREE(p.rgba);
		}
		_pending.resize(0);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			if (!_staging[i].buffer.is_null())
				backend.destroy_resource(_staging[i].buffer);
			_staging[i] = {};
		}

		if (!_texture.is_null())
			backend.destroy_texture(_texture);
		_texture	  = {};
		_gpu_index	  = 0;
		_width		  = 0;
		_height		  = 0;
		_frame		  = 0;
		_transitioned = false;
		_entries.clear();
		_metrics.clear();
		_shelves.clear();
	}

	gfx_texture_handle glyph_atlas_t::get_texture() const
	{
		return _texture;
	}

	gpu_index_t glyph_atlas_t::get_gpu_index() const
	{
		return _gpu_index;
	}

	u32 glyph_atlas_t::get_width() const
	{
		return _width;
	}

	u32 glyph_atlas_t::get_height() const
	{
		return _height;
	}

	bool glyph_atlas_t::is_initialized() const
	{
		return !_texture.is_null();
	}

	void glyph_atlas_t::tick(u32 frame_index)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		_frame = frame_index;
	}

	size_metrics_t glyph_atlas_t::request_size_metrics(const font_runtime_t* font, u32 px_size)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(font != nullptr && font->face != nullptr);
		const u64 key = make_size_key(font->face_id, px_size);

		auto it = _metrics.find(key);
		if (it != _metrics.end())
			return it->second;

		const stbtt_fontinfo* fi	= static_cast<const stbtt_fontinfo*>(font->face);
		const f32			  scale = stbtt_ScaleForPixelHeight(const_cast<stbtt_fontinfo*>(fi), static_cast<f32>(px_size));

		size_metrics_t m;
		m.ascent_px		 = static_cast<f32>(font->ascent) * scale;
		m.descent_px	 = static_cast<f32>(font->descent) * scale;
		m.line_height_px = static_cast<f32>(font->ascent - font->descent + font->line_gap) * scale;
		m.px_height		 = static_cast<f32>(px_size);
		_metrics[key]	 = m;
		return m;
	}

	f32 glyph_atlas_t::get_kern_advance(const font_runtime_t* font, u32 prev_cp, u32 next_cp, u32 px_size) const
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(font != nullptr && font->face != nullptr);
		if (prev_cp == 0)
			return 0.0f;
		const stbtt_fontinfo* fi	= static_cast<const stbtt_fontinfo*>(font->face);
		const f32			  scale = stbtt_ScaleForPixelHeight(const_cast<stbtt_fontinfo*>(fi), static_cast<f32>(px_size));
		const i32			  kern	= stbtt_GetCodepointKernAdvance(const_cast<stbtt_fontinfo*>(fi), static_cast<i32>(prev_cp), static_cast<i32>(next_cp));
		return static_cast<f32>(kern) * scale;
	}

	bool glyph_atlas_t::allocate_slot(i32 w, i32 h, i16& out_x, i16& out_y)
	{
		const i32 slot_w = w + SHELF_PADDING_X;
		const i32 slot_h = h + SHELF_PADDING_Y;
		const i32 max_w	 = static_cast<i32>(_width);
		const i32 max_h	 = static_cast<i32>(_height);

		i32 best_idx = -1;
		i32 best_h	 = max_h + 1;
		for (size_t i = 0; i < _shelves.size(); ++i)
		{
			const shelf_t& s = _shelves[i];
			if (s.height >= h && (s.x_cursor + slot_w) <= max_w && s.height < best_h)
			{
				best_idx = static_cast<i32>(i);
				best_h	 = s.height;
			}
		}

		if (best_idx >= 0)
		{
			shelf_t& s = _shelves[best_idx];
			out_x	   = static_cast<i16>(s.x_cursor);
			out_y	   = static_cast<i16>(s.y);
			s.x_cursor += slot_w;
			return true;
		}

		i32 next_y = 0;
		if (!_shelves.empty())
		{
			const shelf_t& last = _shelves.back();
			next_y				= last.y + last.height;
		}

		if (next_y + slot_h > max_h || slot_w > max_w)
			return false;

		shelf_t s;
		s.y		   = next_y;
		s.height   = slot_h;
		s.x_cursor = slot_w;
		_shelves.push_back(s);

		out_x = 0;
		out_y = static_cast<i16>(next_y);
		return true;
	}

	const glyph_entry_t* glyph_atlas_t::request_glyph(const font_runtime_t* font, u32 codepoint, u32 px_size, glyph_raster_mode_e mode)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(font != nullptr && font->face != nullptr);
		SFG_ASSERT(!_texture.is_null());

		const u64 key = make_glyph_key(font->face_id, codepoint, px_size, mode);

		auto it = _entries.find(key);
		if (it != _entries.end())
		{
			it->second.last_used_frame = _frame;
			return &it->second;
		}

		stbtt_fontinfo* fi		  = static_cast<stbtt_fontinfo*>(font->face);
		const f32		scale_y	  = stbtt_ScaleForPixelHeight(fi, static_cast<f32>(px_size));
		const f32		scale_x	  = scale_y * 3.0f;
		const i32		glyph_idx = stbtt_FindGlyphIndex(fi, static_cast<i32>(codepoint));

		i32 advance_w = 0, lsb = 0;
		stbtt_GetGlyphHMetrics(fi, glyph_idx, &advance_w, &lsb);
		const f32 advance_px = static_cast<f32>(advance_w) * scale_y;

		glyph_entry_t entry	  = {};
		entry.advance_x		  = advance_px;
		entry.last_used_frame = _frame;

		i32 dst_w_pixels = 0;
		i32 dst_h_pixels = 0;
		f32 left_bearing = static_cast<f32>(lsb) * scale_y;
		f32 top_bearing	 = 0.0f;
		u8* rgba		 = nullptr;

		if (glyph_idx != 0)
		{
			if (mode == glyph_raster_mode_e::lcd)
			{
				i32 ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
				stbtt_GetGlyphBitmapBoxSubpixel(fi, glyph_idx, scale_x, scale_y, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);

				const i32 tight_sub_w = ix1 - ix0;
				const i32 bmp_h		  = iy1 - iy0;
				const i32 sub_w		  = tight_sub_w + LCD_FILTER_RADIUS * 2;
				dst_w_pixels		  = (sub_w + 2) / 3;
				dst_h_pixels		  = bmp_h;
				left_bearing		  = static_cast<f32>(ix0 - LCD_FILTER_RADIUS) / 3.0f;
				top_bearing			  = static_cast<f32>(iy0);

				if (tight_sub_w > 0 && dst_w_pixels > 0 && dst_h_pixels > 0)
				{
					const u32 sub_buf_bytes = static_cast<u32>(sub_w) * static_cast<u32>(bmp_h);
					u8*		  sub			= static_cast<u8*>(SFG_MALLOC(sub_buf_bytes));
					SFG_MEMSET(sub, 0, sub_buf_bytes);
					stbtt_MakeGlyphBitmapSubpixel(fi, sub + LCD_FILTER_RADIUS, tight_sub_w, bmp_h, sub_w, scale_x, scale_y, 0.0f, 0.0f, glyph_idx);

					const u32 rgba_bytes = static_cast<u32>(dst_w_pixels) * static_cast<u32>(dst_h_pixels) * ATLAS_BPP;
					rgba				 = static_cast<u8*>(SFG_MALLOC(rgba_bytes));

					for (i32 y = 0; y < dst_h_pixels; ++y)
					{
						const u8* src_row = sub + static_cast<size_t>(y) * static_cast<size_t>(sub_w);
						u8*		  dst_row = rgba + static_cast<size_t>(y) * static_cast<size_t>(dst_w_pixels) * ATLAS_BPP;
						for (i32 x = 0; x < dst_w_pixels; ++x)
						{
							const i32 sx = x * 3;
							const u8  r	 = (sx + 0) < sub_w ? filter_lcd_sample(src_row, sub_w, sx + 0) : 0;
							const u8  g	 = (sx + 1) < sub_w ? filter_lcd_sample(src_row, sub_w, sx + 1) : 0;
							const u8  b	 = (sx + 2) < sub_w ? filter_lcd_sample(src_row, sub_w, sx + 2) : 0;
							u8		  a	 = r;
							if (g > a)
								a = g;
							if (b > a)
								a = b;
							dst_row[x * 4 + 0] = r;
							dst_row[x * 4 + 1] = g;
							dst_row[x * 4 + 2] = b;
							dst_row[x * 4 + 3] = a;
						}
					}
					SFG_FREE(sub);
				}
			}
			else
			{
				i32 xoff = 0, yoff = 0;
				u8* sdf		 = stbtt_GetGlyphSDF(fi, scale_y, glyph_idx, SDF_PADDING, SDF_ONEDGE_VALUE, SDF_DIST_SCALE, &dst_w_pixels, &dst_h_pixels, &xoff, &yoff);
				left_bearing = static_cast<f32>(xoff);
				top_bearing	 = static_cast<f32>(yoff);

				if (sdf != nullptr && dst_w_pixels > 0 && dst_h_pixels > 0)
				{
					const u32 rgba_bytes = static_cast<u32>(dst_w_pixels) * static_cast<u32>(dst_h_pixels) * ATLAS_BPP;
					rgba				 = static_cast<u8*>(SFG_MALLOC(rgba_bytes));
					for (i32 y = 0; y < dst_h_pixels; ++y)
					{
						const u8* src_row = sdf + static_cast<size_t>(y) * static_cast<size_t>(dst_w_pixels);
						u8*		  dst_row = rgba + static_cast<size_t>(y) * static_cast<size_t>(dst_w_pixels) * ATLAS_BPP;
						for (i32 x = 0; x < dst_w_pixels; ++x)
						{
							const u8 v		   = src_row[x];
							dst_row[x * 4 + 0] = v;
							dst_row[x * 4 + 1] = v;
							dst_row[x * 4 + 2] = v;
							dst_row[x * 4 + 3] = v;
						}
					}
				}
				if (sdf != nullptr)
					stbtt_FreeSDF(sdf, nullptr);
			}
		}

		entry.left_bearing = left_bearing;
		entry.top_bearing  = top_bearing;
		entry.width		   = static_cast<i16>(dst_w_pixels);
		entry.height	   = static_cast<i16>(dst_h_pixels);

		if (rgba == nullptr || dst_w_pixels <= 0 || dst_h_pixels <= 0 || glyph_idx == 0)
		{
			entry.atlas_x = 0;
			entry.atlas_y = 0;
			entry.uv_x = entry.uv_y = entry.uv_w = entry.uv_h = 0.0f;
			_entries[key]									  = entry;
			return &_entries[key];
		}

		const i32 upload_w	   = dst_w_pixels + GLYPH_UPLOAD_PADDING * 2;
		const i32 upload_h	   = dst_h_pixels + GLYPH_UPLOAD_PADDING * 2;
		const u32 upload_bytes = static_cast<u32>(upload_w) * static_cast<u32>(upload_h) * ATLAS_BPP;
		u8*		  upload_rgba  = static_cast<u8*>(SFG_MALLOC(upload_bytes));
		SFG_MEMSET(upload_rgba, 0, upload_bytes);
		for (i32 y = 0; y < dst_h_pixels; ++y)
		{
			const u8* src_row = rgba + static_cast<size_t>(y) * static_cast<size_t>(dst_w_pixels) * ATLAS_BPP;
			u8*		  dst_row = upload_rgba + (static_cast<size_t>(y + GLYPH_UPLOAD_PADDING) * static_cast<size_t>(upload_w) + GLYPH_UPLOAD_PADDING) * ATLAS_BPP;
			SFG_MEMCPY(dst_row, src_row, static_cast<size_t>(dst_w_pixels) * ATLAS_BPP);
		}
		SFG_FREE(rgba);
		rgba = upload_rgba;

		i16 ax = 0, ay = 0;
		if (!allocate_slot(upload_w, upload_h, ax, ay))
		{
			SFG_FREE(rgba);
			SFG_ASSERT(!"glyph atlas full");
			entry.atlas_x = 0;
			entry.atlas_y = 0;
			_entries[key] = entry;
			return &_entries[key];
		}

		entry.atlas_x = static_cast<i16>(ax + GLYPH_UPLOAD_PADDING);
		entry.atlas_y = static_cast<i16>(ay + GLYPH_UPLOAD_PADDING);
		entry.uv_x	  = static_cast<f32>(entry.atlas_x) / static_cast<f32>(_width);
		entry.uv_y	  = static_cast<f32>(entry.atlas_y) / static_cast<f32>(_height);
		entry.uv_w	  = static_cast<f32>(dst_w_pixels) / static_cast<f32>(_width);
		entry.uv_h	  = static_cast<f32>(dst_h_pixels) / static_cast<f32>(_height);

		pending_upload_t up;
		up.atlas_x = ax;
		up.atlas_y = ay;
		up.width   = static_cast<i16>(upload_w);
		up.height  = static_cast<i16>(upload_h);
		up.rgba	   = rgba;
		_pending.push_back(up);

		_entries[key] = entry;
		return &_entries[key];
	}

	void glyph_atlas_t::drain_uploads(u8 frame_slot)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (_pending.empty())
			return;

		SFG_ASSERT(frame_slot < BACK_BUFFER_COUNT);
		staging_slot_t& slot = _staging[frame_slot];
		SFG_ASSERT(slot.mapped != nullptr);

		u32 cursor = 0;
		for (pending_upload_t& p : _pending)
		{
			const u32 row_pitch		= align_up(static_cast<u32>(p.width) * ATLAS_BPP, PITCH_ALIGNMENT);
			const u32 needed		= row_pitch * static_cast<u32>(p.height);
			const u32 placed_offset = align_up(cursor, PLACEMENT_ALIGNMENT);

			SFG_ASSERT(placed_offset + needed <= slot.capacity);

			u8* dst = slot.mapped + placed_offset;
			for (i32 y = 0; y < p.height; ++y)
			{
				const u8* src_row = p.rgba + static_cast<size_t>(y) * static_cast<size_t>(p.width) * ATLAS_BPP;
				u8*		  dst_row = dst + static_cast<size_t>(y) * static_cast<size_t>(row_pitch);
				SFG_MEMCPY(dst_row, src_row, static_cast<size_t>(p.width) * ATLAS_BPP);
			}

			texture_region_upload_desc_t desc = {};
			desc.dst_texture				  = _texture;
			desc.src_buffer					  = slot.buffer;
			desc.src_offset					  = placed_offset;
			desc.src_row_pitch				  = row_pitch;
			desc.dst_x						  = static_cast<u16>(p.atlas_x);
			desc.dst_y						  = static_cast<u16>(p.atlas_y);
			desc.width						  = static_cast<u16>(p.width);
			desc.height						  = static_cast<u16>(p.height);
			desc.bpp						  = ATLAS_BPP;
			desc.dst_mip					  = 0;
			desc.target_states				  = resource_state_ps_resource;
			render_resources_t::get().enqueue_texture_region_upload(desc);

			SFG_FREE(p.rgba);
			cursor = placed_offset + needed;
		}

		_transitioned = true;
		_pending.resize(0);
	}
}
