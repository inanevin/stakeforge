// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/freetype_runtime.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace sfg
{
	bool font_loader_t::load(resource_entry_t&, resource_context_t&, ostream_t&)
	{
		return false;
	}

	bool font_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		font_runtime_t*	   runtime = mem.get<font_runtime_t>(entry.runtime);
		*runtime				   = {};

		istream_t stream;
		stream.open(entry.load_data.data, entry.load_data.size);

		stream >> runtime->ttf_size;
		runtime->ttf_data = stream.get_data_current();
		return true;
	}

	create_internals_result_e font_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		font_runtime_t*	   runtime = mem.get<font_runtime_t>(entry.runtime);

		const u8* ttf_src  = runtime->ttf_data;
		runtime->ttf_chunk = mem.allocate_bytes(runtime->ttf_size, alignof(u8));
		u8* ttf_dst		   = mem.get<u8>(runtime->ttf_chunk);
		SFG_MEMCPY(ttf_dst, ttf_src, runtime->ttf_size);
		runtime->ttf_data = ttf_dst;

		FT_Face	   face	   = nullptr;
		FT_Library library = static_cast<FT_Library>(freetype_runtime_t::get_library());
		if (FT_New_Memory_Face(library, runtime->ttf_data, static_cast<FT_Long>(runtime->ttf_size), 0, &face) != 0)
		{
			SFG_ERR("FT_New_Memory_Face failed");
			mem.free(runtime->ttf_chunk);
			*runtime = {};
			return create_internals_result_e::failed;
		}

		runtime->face	 = face;
		runtime->face_id = entry.hash;
		runtime->ascent	 = static_cast<i32>(face->ascender);
		runtime->descent = static_cast<i32>(face->descender);
		runtime->height	 = static_cast<i32>(face->height);
		return create_internals_result_e::ready;
	}

	void font_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		font_runtime_t*	   font = mem.get<font_runtime_t>(entry.runtime);
		if (font->face != nullptr)
		{
			FT_Done_Face(static_cast<FT_Face>(font->face));
		}
		if (font->ttf_chunk)
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
		.initial_load_offset = 0,
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.async_load			 = false,
		.load				 = font_loader_t::load,
		.load_v2			 = font_loader_t::load,
		.create_internals	 = font_loader_t::create_internals,
		.destroy_internals	 = font_loader_t::destroy_internals,
	};
}
