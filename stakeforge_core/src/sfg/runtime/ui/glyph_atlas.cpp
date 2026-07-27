// Copyright (c) 2025 Inan Evin

#include "glyph_atlas.hpp"
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/font.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

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

		inline f32 from_26dot6(FT_Pos v)
		{
			return static_cast<f32>(v) / 64.0f;
		}

		inline const u8* bitmap_row(const FT_Bitmap& bitmap, i32 y)
		{
			const i32 pitch = bitmap.pitch;
			return pitch >= 0 ? bitmap.buffer + static_cast<size_t>(y) * static_cast<size_t>(pitch) : bitmap.buffer + static_cast<size_t>(bitmap.rows - 1 - y) * static_cast<size_t>(-pitch);
		}
	}

	glyph_atlas_t::~glyph_atlas_t() = default;

	void glyph_atlas_t::init(const glyph_atlas_config_t& cfg)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(_texture.is_null());
		SFG_ASSERT(cfg.width > 0 && cfg.height > 0);

		render_resources_t& render_resources = render_resources_t::get();

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
		_texture = render_resources.enqueue_create_texture(tdesc);

		const u32 staging_bytes = align_up(cfg.staging_budget_bytes, PLACEMENT_ALIGNMENT);
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			resource_desc_t sdesc = {};
			sdesc.size			  = staging_bytes;
			sdesc.flags			  = resource_flags::rf_cpu_visible;
			sdesc.set_name("ui_glyph_staging");
			_staging[i].buffer	 = render_resources.enqueue_create_resource(sdesc);
			_staging[i].capacity = staging_bytes;
		}

		_entries.reserve(cfg.glyph_initial_capacity);
		_metrics.reserve(cfg.size_metric_initial_capacity);
		_shelves.reserve(cfg.shelf_initial_capacity);
		_pending.reserve(cfg.pending_upload_initial_capacity);
		_transitioned = false;
	}

	void glyph_atlas_t::uninit()
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		render_resources_t& render_resources = render_resources_t::get();

		for (pending_upload_t& p : _pending)
		{
			SFG_FREE(p.rgba);
		}
		_pending.resize(0);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			if (!_staging[i].buffer.is_null())
				render_resources.enqueue_destroy_resource(_staging[i].buffer);
			_staging[i] = {};
		}

		if (!_texture.is_null())
			render_resources.enqueue_destroy_texture(_texture);
		_texture	  = {};
		_width		  = 0;
		_height		  = 0;
		_frame		  = 0;
		_transitioned = false;
		_entries.clear();
		_metrics.clear();
		_shelves.clear();
	}

	render_resource_handle_t glyph_atlas_t::get_texture() const
	{
		return _texture;
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

		FT_Face face = static_cast<FT_Face>(font->face);
		if (FT_Set_Pixel_Sizes(face, 0, px_size) != 0)
		{
			SFG_ERR("FT_Set_Pixel_Sizes failed");
			return {};
		}

		size_metrics_t m;
		m.ascent_px		 = from_26dot6(face->size->metrics.ascender);
		m.descent_px	 = from_26dot6(face->size->metrics.descender);
		m.line_height_px = from_26dot6(face->size->metrics.height);
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
		FT_Face face = static_cast<FT_Face>(font->face);
		if (!FT_HAS_KERNING(face) || FT_Set_Pixel_Sizes(face, 0, px_size) != 0)
			return 0.0f;

		const FT_UInt prev = FT_Get_Char_Index(face, prev_cp);
		const FT_UInt next = FT_Get_Char_Index(face, next_cp);
		if (prev == 0 || next == 0)
			return 0.0f;

		FT_Vector kern = {};
		if (FT_Get_Kerning(face, prev, next, FT_KERNING_DEFAULT, &kern) != 0)
			return 0.0f;
		return from_26dot6(kern.x);
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

		FT_Face face = static_cast<FT_Face>(font->face);
		if (FT_Set_Pixel_Sizes(face, 0, px_size) != 0)
		{
			SFG_ERR("FT_Set_Pixel_Sizes failed");
			glyph_entry_t entry = {};
			_entries[key]		= entry;
			return &_entries[key];
		}

		const FT_UInt glyph_idx = FT_Get_Char_Index(face, codepoint);

		glyph_entry_t entry	  = {};
		entry.last_used_frame = _frame;

		i32 dst_w_pixels = 0;
		i32 dst_h_pixels = 0;
		f32 left_bearing = 0.0f;
		f32 top_bearing	 = 0.0f;
		u8* rgba		 = nullptr;

		if (glyph_idx != 0)
		{
			FT_Int32	   load_flags  = FT_LOAD_DEFAULT;
			FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;
			if (mode == glyph_raster_mode_e::lcd)
			{
				load_flags |= FT_LOAD_TARGET_LCD;
				render_mode = FT_RENDER_MODE_LCD;
			}
			else if (mode == glyph_raster_mode_e::grayscale)
			{
				load_flags |= FT_LOAD_TARGET_NORMAL;
				render_mode = FT_RENDER_MODE_NORMAL;
			}
			else
			{
				load_flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
				render_mode = FT_RENDER_MODE_SDF;
			}

			bool rendered = false;
			if (FT_Load_Glyph(face, glyph_idx, load_flags) == 0)
			{
				FT_GlyphSlot slot = face->glyph;
				entry.advance_x	  = from_26dot6(slot->advance.x);
				rendered		  = FT_Render_Glyph(slot, render_mode) == 0;
			}

			if (rendered && face->glyph->bitmap.buffer != nullptr)
			{
				FT_GlyphSlot	 slot = face->glyph;
				const FT_Bitmap& bmp  = slot->bitmap;
				left_bearing		  = static_cast<f32>(slot->bitmap_left);
				top_bearing			  = -static_cast<f32>(slot->bitmap_top);
				dst_h_pixels		  = static_cast<i32>(bmp.rows);
				dst_w_pixels		  = mode == glyph_raster_mode_e::lcd ? static_cast<i32>((bmp.width + 2) / 3) : static_cast<i32>(bmp.width);

				if (dst_w_pixels > 0 && dst_h_pixels > 0 && bmp.buffer != nullptr)
				{
					const u32 rgba_bytes = static_cast<u32>(dst_w_pixels) * static_cast<u32>(dst_h_pixels) * ATLAS_BPP;
					rgba				 = static_cast<u8*>(SFG_MALLOC(rgba_bytes));
					for (i32 y = 0; y < dst_h_pixels; ++y)
					{
						const u8* src_row = bitmap_row(bmp, y);
						u8*		  dst_row = rgba + static_cast<size_t>(y) * static_cast<size_t>(dst_w_pixels) * ATLAS_BPP;
						for (i32 x = 0; x < dst_w_pixels; ++x)
						{
							if (mode == glyph_raster_mode_e::lcd)
							{
								const i32 sx = x * 3;
								const u8  r	 = sx + 0 < static_cast<i32>(bmp.width) ? src_row[sx + 0] : 0;
								const u8  g	 = sx + 1 < static_cast<i32>(bmp.width) ? src_row[sx + 1] : 0;
								const u8  b	 = sx + 2 < static_cast<i32>(bmp.width) ? src_row[sx + 2] : 0;
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
							else
							{
								const u8 v		   = src_row[x];
								dst_row[x * 4 + 0] = v;
								dst_row[x * 4 + 1] = v;
								dst_row[x * 4 + 2] = v;
								dst_row[x * 4 + 3] = v;
							}
						}
					}
				}
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
		SFG_ASSERT(!slot.buffer.is_null());

		u32 cursor = 0;
		for (pending_upload_t& p : _pending)
		{
			const u32 row_pitch		= align_up(static_cast<u32>(p.width) * ATLAS_BPP, PITCH_ALIGNMENT);
			const u32 needed		= row_pitch * static_cast<u32>(p.height);
			const u32 placed_offset = align_up(cursor, PLACEMENT_ALIGNMENT);

			SFG_ASSERT(placed_offset + needed <= slot.capacity);

			u8* upload = static_cast<u8*>(SFG_MALLOC(needed));
			SFG_ASSERT(upload != nullptr);
			SFG_MEMSET(upload, 0, needed);
			for (i32 y = 0; y < p.height; ++y)
			{
				const u8* src_row = p.rgba + static_cast<size_t>(y) * static_cast<size_t>(p.width) * ATLAS_BPP;
				u8*		  dst_row = upload + static_cast<size_t>(y) * static_cast<size_t>(row_pitch);
				SFG_MEMCPY(dst_row, src_row, static_cast<size_t>(p.width) * ATLAS_BPP);
			}

			render_resources_t::get().enqueue_data_upload({
				.data		= upload,
				.resource	= slot.buffer,
				.dst_offset = placed_offset,
				.data_size	= needed,
			});
			SFG_FREE(upload);

			render_texture_region_upload_desc_t desc = {};
			desc.dst_texture						 = _texture;
			desc.src_buffer							 = slot.buffer;
			desc.src_offset							 = placed_offset;
			desc.src_row_pitch						 = row_pitch;
			desc.dst_x								 = static_cast<u16>(p.atlas_x);
			desc.dst_y								 = static_cast<u16>(p.atlas_y);
			desc.width								 = static_cast<u16>(p.width);
			desc.height								 = static_cast<u16>(p.height);
			desc.bpp								 = ATLAS_BPP;
			desc.dst_mip							 = 0;
			desc.target_states						 = resource_state_ps_resource;
			render_resources_t::get().enqueue_texture_region_upload(desc);

			SFG_FREE(p.rgba);
			cursor = placed_offset + needed;
		}

		_transitioned = true;
		_pending.resize(0);
	}
}
