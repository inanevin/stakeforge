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

#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_project.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_payload_type.hpp>
#include <sfg/serialization/serialization.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace sfg
{
#define EDITOR_THUMBNAIL_SIZE	 256
#define EDITOR_THUMBNAIL_FONT_PX 150

	namespace
	{
		bool fill_texture_thumbnail(const editor_asset_t& asset, vector_t<u8>& pixels)
		{
			if (asset.source_relative.empty())
				return false;

			string_t source_full_path = file_system_t::get_absolute_path(editor_project_t::get()._runtime.assets_path.c_str());
			source_full_path += asset.source_relative;
			if (!file_system_t::exists(source_full_path.c_str()))
				return false;

			vec2u16_t source_size = {};
			u8*		  source	  = static_cast<u8*>(image_util_t::load_from_file_ch(source_full_path.c_str(), source_size, 4));
			if (source == nullptr)
				return false;

			pixels.resize(static_cast<size_t>(EDITOR_THUMBNAIL_SIZE) * static_cast<size_t>(EDITOR_THUMBNAIL_SIZE) * 4);
			const bool resized = image_util_t::resize_rgba8(
				{.data = source, .size = static_cast<size_t>(source_size.x) * static_cast<size_t>(source_size.y) * 4}, source_size, {.data = pixels.data(), .size = pixels.size()}, vec2u16_t(EDITOR_THUMBNAIL_SIZE, EDITOR_THUMBNAIL_SIZE));
			image_util_t::free(source);
			return resized;
		}

		bool fill_font_thumbnail(const editor_asset_t& asset, vector_t<u8>& pixels)
		{
			if (asset.source_relative.empty())
				return false;

			string_t source_full_path = file_system_t::get_absolute_path(editor_project_t::get()._runtime.assets_path.c_str());
			source_full_path += asset.source_relative;
			if (!file_system_t::exists(source_full_path.c_str()))
				return false;

			FT_Library library = nullptr;
			if (FT_Init_FreeType(&library) != 0)
				return false;

			FT_Face face = nullptr;
			if (FT_New_Face(library, source_full_path.c_str(), 0, &face) != 0)
			{
				FT_Done_FreeType(library);
				return false;
			}

			if (FT_Set_Pixel_Sizes(face, 0, EDITOR_THUMBNAIL_FONT_PX) != 0)
			{
				FT_Done_Face(face);
				FT_Done_FreeType(library);
				return false;
			}

			const char text[]	  = {'A', 'a'};
			i32		   pen_x	  = 0;
			i32		   min_x	  = 0;
			i32		   max_x	  = 0;
			i32		   top		  = 0;
			i32		   bottom	  = 0;
			bool	   has_bounds = false;
			for (char c : text)
			{
				if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) != 0)
					continue;

				FT_GlyphSlot slot		  = face->glyph;
				const i32	 glyph_min_x  = pen_x + slot->bitmap_left;
				const i32	 glyph_max_x  = pen_x + slot->bitmap_left + static_cast<i32>(slot->bitmap.width);
				const i32	 glyph_bottom = static_cast<i32>(slot->bitmap.rows) - slot->bitmap_top;
				if (!has_bounds || glyph_min_x < min_x)
					min_x = glyph_min_x;
				if (!has_bounds || glyph_max_x > max_x)
					max_x = glyph_max_x;
				if (!has_bounds || slot->bitmap_top > top)
					top = slot->bitmap_top;
				if (!has_bounds || glyph_bottom > bottom)
					bottom = glyph_bottom;
				has_bounds = true;
				pen_x += static_cast<i32>(slot->advance.x >> 6);
			}

			if (!has_bounds)
			{
				FT_Done_Face(face);
				FT_Done_FreeType(library);
				return false;
			}

			pixels.resize(static_cast<size_t>(EDITOR_THUMBNAIL_SIZE) * static_cast<size_t>(EDITOR_THUMBNAIL_SIZE) * 4);
			for (size_t i = 0; i < pixels.size(); i += 4)
			{
				pixels[i + 0] = 10;
				pixels[i + 1] = 10;
				pixels[i + 2] = 12;
				pixels[i + 3] = 255;
			}

			const i32 bounds_w = max_x - min_x;
			const i32 bounds_h = top + bottom;
			pen_x			   = (EDITOR_THUMBNAIL_SIZE - bounds_w) / 2 - min_x;
			const i32 baseline = (EDITOR_THUMBNAIL_SIZE - bounds_h) / 2 + top;

			for (char c : text)
			{
				if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) != 0)
					continue;

				FT_GlyphSlot	 slot  = face->glyph;
				const FT_Bitmap& bmp   = slot->bitmap;
				const i32		 dst_x = pen_x + slot->bitmap_left;
				const i32		 dst_y = baseline - slot->bitmap_top;
				for (i32 y = 0; y < static_cast<i32>(bmp.rows); ++y)
				{
					const u8* row = bmp.pitch >= 0 ? bmp.buffer + y * bmp.pitch : bmp.buffer + (static_cast<i32>(bmp.rows) - 1 - y) * -bmp.pitch;
					for (i32 x = 0; x < static_cast<i32>(bmp.width); ++x)
					{
						const i32 px = dst_x + x;
						const i32 py = dst_y + y;
						if (px < 0 || py < 0 || px >= EDITOR_THUMBNAIL_SIZE || py >= EDITOR_THUMBNAIL_SIZE)
							continue;

						const u8 coverage = row[x];
						u8*		 dst	  = pixels.data() + (static_cast<size_t>(py) * EDITOR_THUMBNAIL_SIZE + static_cast<size_t>(px)) * 4;
						dst[0]			  = static_cast<u8>((static_cast<u32>(dst[0]) * (255 - coverage) + 238u * coverage) / 255u);
						dst[1]			  = static_cast<u8>((static_cast<u32>(dst[1]) * (255 - coverage) + 238u * coverage) / 255u);
						dst[2]			  = static_cast<u8>((static_cast<u32>(dst[2]) * (255 - coverage) + 238u * coverage) / 255u);
					}
				}

				pen_x += static_cast<i32>(slot->advance.x >> 6);
			}

			FT_Done_Face(face);
			FT_Done_FreeType(library);
			return true;
		}

	}

	void editor_asset_thumbnailer_t::generate_thumbnail(const editor_asset_t& asset, const char* display_name)
	{
		if (asset.thumbnail_guid == NULL_SID)
			return;

		if (get_builtin_thumbnail_guid(asset.asset_type) != NULL_SID)
			return;

		if (is_renderable_thumbnail(asset.asset_type))
			return;

		vector_t<u8> pixels = {};

		pixels.reserve(256 * 256 * 4);

		bool filled = false;

		if (asset.asset_type == editor_asset_type_e::texture)
			filled = fill_texture_thumbnail(asset, pixels);
		else if (asset.asset_type == editor_asset_type_e::font)
			filled = fill_font_thumbnail(asset, pixels);

		if (filled)
			save_thumbnail(asset, {.data = pixels.data(), .size = pixels.size()}, display_name);
	}

	bool editor_asset_thumbnailer_t::save_thumbnail(const editor_asset_t& asset, span_t<u8> pixels, const char* display_name)
	{
		if (asset.thumbnail_guid == NULL_SID || pixels.size == 0)
			return false;

		const texture_cook_config_t config = {
			.size			  = vec2u16_t(EDITOR_THUMBNAIL_SIZE, EDITOR_THUMBNAIL_SIZE),
			.payload_type	  = texture_payload_type_e::png,
			.generate_mipmaps = false,
			.is_linear		  = false,
			.use_streaming	  = false,
			.force_4_channels = true,
		};

		resource_header_t header  = {};
		ostream_t		  payload = {};

		if (!texture_cooker::cook_from_data(config, pixels, header, payload))
		{
			SFG_ERR("failed to cook thumbnail for asset {0}", asset.guid);
			return false;
		}

		header.source_tick		 = 0;
		header.file_source_ticks = 0;
		string_t name			 = "thumb_";

		if (display_name != nullptr && display_name[0] != '\0')
			name += display_name;
		header.set_debug_name(name.c_str());

		const string_t cache_dir  = editor_project_t::get()._runtime.cache_path;
		const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);
		if (!file_system_t::ensure_directory(cache_dir.c_str()))
		{
			SFG_ERR("failed to create asset cache directory {0}", cache_dir.c_str());
			return false;
		}

		ostream_t stream = header.make_stream(payload);
		if (!serializer_t::save_to_file_atomic(cache_path.c_str(), stream))
		{
			SFG_ERR("failed to save thumbnail {0}", cache_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_thumbnailer_t::is_renderable_thumbnail(editor_asset_type_e asset_type)
	{
		switch (asset_type)
		{
		case editor_asset_type_e::animation:
		case editor_asset_type_e::material:
		case editor_asset_type_e::mesh:
		case editor_asset_type_e::hdr_skybox:
		case editor_asset_type_e::prefab:
		case editor_asset_type_e::physics_collision_mesh:
			return true;
		default:
			return false;
		}
	}

	sid_t editor_asset_thumbnailer_t::make_thumbnail_guid(editor_asset_type_e asset_type, span_t<const sid_t> pending_guids)
	{
		const sid_t builtin_guid = get_builtin_thumbnail_guid(asset_type);
		return builtin_guid != NULL_SID ? builtin_guid : editor_asset_util_t::generate_unique_asset_guid(pending_guids);
	}

	sid_t editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(editor_asset_type_e asset_type)
	{
		switch (asset_type)
		{
		case editor_asset_type_e::audio:
			return "editor/resource_pack/textures/thumbnails/audio.png"_hs;
		case editor_asset_type_e::shader:
			return "editor/resource_pack/textures/thumbnails/shader.png"_hs;
		case editor_asset_type_e::texture_sampler:
			return "editor/resource_pack/textures/thumbnails/texture_sampler.png"_hs;
		case editor_asset_type_e::physical_material:
			return "editor/resource_pack/textures/thumbnails/physical_material.png"_hs;
		case editor_asset_type_e::world:
			return "editor/resource_pack/textures/thumbnails/world.png"_hs;
		case editor_asset_type_e::animation:
			return "editor/resource_pack/textures/thumbnails/animation_clip.png"_hs;
		case editor_asset_type_e::animation_graph:
			return "editor/resource_pack/textures/thumbnails/animation_graph.png"_hs;
		case editor_asset_type_e::skeleton:
			return "editor/resource_pack/textures/thumbnails/skeleton.png"_hs;

		default:
			return NULL_SID;
		}
	}

#undef EDITOR_THUMBNAIL_SIZE
#undef EDITOR_THUMBNAIL_FONT_PX
}
