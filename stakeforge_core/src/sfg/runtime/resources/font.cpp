// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/freetype_runtime.hpp>
#include <sfg/serialization/compression.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace sfg
{
	bool font_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
			return false;

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);
		if (payload.empty())
			return false;

		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		font_runtime_t*	   font = mem.get<font_runtime_t>(entry.runtime);
		*font					= {};

		payload >> font->ttf_size;
		const u8* ttf_src = payload.get_data_current();

		font->ttf_chunk = mem.allocate_bytes(font->ttf_size, alignof(u8));
		u8* ttf_dst		= mem.get<u8>(font->ttf_chunk);
		SFG_MEMCPY(ttf_dst, ttf_src, font->ttf_size);
		font->ttf_data = ttf_dst;

		FT_Face	   face	   = nullptr;
		FT_Library library = static_cast<FT_Library>(freetype_runtime_t::get_library());
		if (FT_New_Memory_Face(library, font->ttf_data, static_cast<FT_Long>(font->ttf_size), 0, &face) != 0)
		{
			SFG_ERR("FT_New_Memory_Face failed");
			mem.free(font->ttf_chunk);
			*font = {};
			return false;
		}

		font->face	  = face;
		font->face_id = entry.hash;
		font->ascent  = static_cast<i32>(face->ascender);
		font->descent = static_cast<i32>(face->descender);
		font->height  = static_cast<i32>(face->height);
		return true;
	}

	void font_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		font_runtime_t*	   font = mem.get<font_runtime_t>(entry.runtime);
		FT_Done_Face(static_cast<FT_Face>(font->face));
		mem.free(font->ttf_chunk);
		*font = {};
	}

	const resource_type_desc_t font_resource_desc = {
		.type				 = resource_type_e::font,
		.runtime_size		 = sizeof(font_runtime_t),
		.runtime_alignment	 = alignof(font_runtime_t),
		.internals_size		 = sizeof(font_internals_t),
		.internals_alignment = alignof(font_internals_t),
		.wire_magic			 = font_loader_t::WIRE_MAGIC,
		.wire_version		 = font_loader_t::WIRE_VERSION,
		.use_async_load		 = false,
		.load				 = font_loader_t::load,
		.unload				 = font_loader_t::unload,
	};
}
