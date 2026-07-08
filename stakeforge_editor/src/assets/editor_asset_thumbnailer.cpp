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

#include "assets/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/char_util.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
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
		u64 get_asset_thumbnail_source_tick(const editor_asset_t& asset)
		{
			u64 result = hashing_t::hash_u64_combine(editor_asset_thumbnailer_t::get_thumbnail_guid(asset.guid), asset.asset_type, asset.source_type, asset.sub_type);
			if (!asset.source_relative.empty())
				result = hashing_t::hash_u64(result, asset.source_relative.data(), asset.source_relative.size());
			if (!asset.embedded_source.empty())
				result = hashing_t::hash_u64(result, asset.embedded_source.data(), asset.embedded_source.size());
			if (!asset.cook_options.empty())
				result = hashing_t::hash_u64(result, asset.cook_options.data(), asset.cook_options.size());
			return result;
		}

		u64 get_asset_thumbnail_file_source_ticks(const editor_asset_t& asset)
		{
			if (asset.source_type != editor_asset_source_type_e::file && asset.source_type != editor_asset_source_type_e::file_blob)
				return 0;
			if (asset.source_relative.empty())
				return 0;

			string_t source_full_path = file_system_t::get_absolute_path(editor_project_t::get()._runtime.assets_path.c_str());
			source_full_path += asset.source_relative;
			if (!file_system_t::exists(source_full_path.c_str()))
				return 0;
			return file_system_t::get_last_modified_ticks(source_full_path.c_str());
		}

		bool is_generated_thumbnail_current(const editor_asset_t& asset, const char* thumbnail_path)
		{
			if (!file_system_t::exists(thumbnail_path))
				return false;

			istream_t stream = serializer_t::load_from_file_slice(thumbnail_path, 0, sizeof(resource_header_t));
			if (stream.empty())
				return false;

			resource_header_t header = {};
			header.deserialize(stream);
			if (header.magic != texture_loader_t::WIRE_MAGIC || header.version != texture_loader_t::WIRE_VERSION)
				return false;
			if (header.source_tick != get_asset_thumbnail_source_tick(asset))
				return false;
			return header.file_source_ticks == get_asset_thumbnail_file_source_ticks(asset);
		}

		bool can_generate_thumbnail(editor_asset_type_e asset_type)
		{
			switch (asset_type)
			{
			case editor_asset_type_e::texture:
			case editor_asset_type_e::font:
				return true;
			default:
				return false;
			}
		}

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

		bool generate_thumbnail_pixels(const editor_asset_t& asset, vector_t<u8>& pixels)
		{
			if (asset.asset_type == editor_asset_type_e::texture)
				return fill_texture_thumbnail(asset, pixels);
			if (asset.asset_type == editor_asset_type_e::font)
				return fill_font_thumbnail(asset, pixels);

			return false;
		}

		bool save_thumbnail(const editor_asset_t& asset, const char* asset_name, vector_t<u8>& pixels)
		{
			const texture_cook_config_t config = {
				.size			  = vec2u16_t(EDITOR_THUMBNAIL_SIZE, EDITOR_THUMBNAIL_SIZE),
				.payload_type	  = texture_payload_type_e::png,
				.generate_mipmaps = false,
				.is_linear		  = false,
				.use_streaming	  = false,
				.force_4_channels = true,
			};

			resource_header_t header = {};
			ostream_t		  payload;
			if (!texture_cooker::cook_from_data(config, {.data = pixels.data(), .size = pixels.size()}, header, payload))
			{
				SFG_ERR("failed to cook thumbnail for asset {0}", asset.guid);
				return false;
			}

			header.source_tick		 = get_asset_thumbnail_source_tick(asset);
			header.file_source_ticks = get_asset_thumbnail_file_source_ticks(asset);
			string_t name			 = "thumb_";

			if (asset_name != nullptr && asset_name[0] != '\0')
				name += asset_name;
			else if (const char* display_name = editor_asset_util_t::find_asset_display_name(asset.guid); display_name != nullptr && display_name[0] != '\0')
				name += display_name;

			header.set_debug_name(name.c_str());

			const string_t cache_dir  = editor_project_t::get()._runtime.cache_path;
			const string_t cache_path = editor_asset_util_t::get_thumbnail_cache_path_for_asset(asset);
			if (!file_system_t::ensure_directory(cache_dir.c_str()))
			{
				SFG_ERR("failed to create asset cache directory {0}", cache_dir.c_str());
				return false;
			}

			ostream_t stream = header.make_stream(payload);
			if (!serializer_t::save_to_file(cache_path.c_str(), stream))
			{
				SFG_ERR("failed to save thumbnail {0}", cache_path.c_str());
				return false;
			}

			return true;
		}

		bool generate_thumbnail(const editor_asset_t& asset, const char* asset_name)
		{
			vector_t<u8> pixels;
			if (!generate_thumbnail_pixels(asset, pixels))
				return false;
			return save_thumbnail(asset, asset_name, pixels);
		}

		bool load_thumbnail_resource(const editor_asset_t& asset)
		{
			const sid_t			thumbnail_guid	 = editor_asset_thumbnailer_t::get_thumbnail_guid(asset.guid);
			resource_manager_t& resource_manager = resource_manager_t::get();
			if (resource_manager.find_entry(thumbnail_guid) == nullptr)
				return resource_manager.load_resource(thumbnail_guid, resource_type_e::texture) != resource_state_e::failed;
			return true;
		}
	}

	editor_asset_thumbnail_t editor_asset_thumbnailer_t::get_thumbnail(const editor_asset_t& asset, const char* asset_name)
	{
		const sid_t builtin_guid = get_builtin_thumbnail_guid(asset.asset_type);
		if (builtin_guid != NULL_SID)
			return {.texture = builtin_guid, .source = editor_asset_thumbnail_source_e::builtin};

		const sid_t thumbnail_guid = get_thumbnail_guid(asset.guid);
		return {.texture = resource_manager_t::get().is_ready(thumbnail_guid) ? thumbnail_guid : NULL_SID, .source = editor_asset_thumbnail_source_e::generated};
	}

	bool editor_asset_thumbnailer_t::ensure(const editor_asset_t& asset, const char* asset_name, bool force)
	{
		if (get_builtin_thumbnail_guid(asset.asset_type) != NULL_SID)
			return true;

		const string_t cache_path = editor_asset_util_t::get_thumbnail_cache_path_for_asset(asset);
		if (!can_generate_thumbnail(asset.asset_type))
		{
			if (file_system_t::exists(cache_path.c_str()) && file_system_t::delete_file(cache_path.c_str()))
				SFG_ERR("failed to delete thumbnail asset {0}", cache_path.c_str());
			return true;
		}

		if (!force && is_generated_thumbnail_current(asset, cache_path.c_str()))
			return true;

		if (generate_thumbnail(asset, asset_name))
			return true;

		if (file_system_t::exists(cache_path.c_str()) && file_system_t::delete_file(cache_path.c_str()))
			SFG_ERR("failed to delete thumbnail asset {0}", cache_path.c_str());
		return false;
	}

	bool editor_asset_thumbnailer_t::ensure_thumbnail_loaded(const editor_asset_t& asset, const char* asset_name)
	{
		if (get_builtin_thumbnail_guid(asset.asset_type) != NULL_SID)
			return true;

		if (!can_generate_thumbnail(asset.asset_type))
			return true;

		const string_t cache_path = editor_asset_util_t::get_thumbnail_cache_path_for_asset(asset);
		if (!is_generated_thumbnail_current(asset, cache_path.c_str()))
			return true;

		return load_thumbnail_resource(asset);
	}

	sid_t editor_asset_thumbnailer_t::get_thumbnail_guid(sid_t asset_guid)
	{
		char  guid_text[32] = {};
		char* guid_text_cur = guid_text;
		if (!char_util::append_u64(guid_text_cur, guid_text + sizeof(guid_text), asset_guid))
			SFG_ASSERT(false);

		string_t key = guid_text;
		key += "_thumb";
		return hashing_t::to_sid(key);
	}

	sid_t editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(editor_asset_type_e asset_type)
	{
		switch (asset_type)
		{
		case editor_asset_type_e::audio:
			return "editor/thumbnails/audio.png"_hs;
		case editor_asset_type_e::shader:
			return "editor/thumbnails/shader.png"_hs;
		case editor_asset_type_e::animation:
			return "editor/thumbnails/animation.png"_hs;
		case editor_asset_type_e::texture_sampler:
			return "editor/thumbnails/texture_sampler.png"_hs;
		case editor_asset_type_e::physical_material:
			return "editor/thumbnails/physical_material.png"_hs;
		default:
			return NULL_SID;
		}
	}

#undef EDITOR_THUMBNAIL_SIZE
#undef EDITOR_THUMBNAIL_FONT_PX
}
