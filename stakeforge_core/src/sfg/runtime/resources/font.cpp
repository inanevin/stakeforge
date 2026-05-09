// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_manager.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/stb/stb_truetype.h>
#include <sfg/data/istream.hpp>

namespace sfg
{
	bool font_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		font_runtime_t*	   runtime = mem.get<font_runtime_t>(entry.runtime);

		istream_t stream;
		stream.open(entry.after_header_data.data, entry.after_header_data.size);

		stream >> runtime->ttf_size;
		runtime->ttf_data = stream.get_data_current();

		chunk_handle32_t face_chunk = mem.allocate_bytes(sizeof(stbtt_fontinfo), alignof(stbtt_fontinfo));
		stbtt_fontinfo*	 fi			= reinterpret_cast<stbtt_fontinfo*>(mem.get(face_chunk.head));

		if (stbtt_InitFont(fi, runtime->ttf_data, stbtt_GetFontOffsetForIndex(runtime->ttf_data, 0)) == 0)
		{
			SFG_ERR(" stbtt_InitFont failed");
			return false;
		}

		runtime->face			= fi;
		runtime->face_id		= entry.hash;
		stbtt_GetFontVMetrics(fi, &runtime->ascent, &runtime->descent, &runtime->line_gap);
		return true;
	}

	create_internals_result_e font_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::ready;
	}

	void font_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		font_runtime_t*	   font = mem.get<font_runtime_t>(entry.runtime);
		
	}

	const resource_type_desc_t font_resource_desc = {
		.type				 = resource_type_e::font,
		.runtime_size		 = sizeof(font_runtime_t),
		.runtime_alignment	 = alignof(font_runtime_t),
		.internals_size		 = sizeof(font_internals_t),
		.internals_alignment = alignof(font_internals_t),
		.wire_magic			 = font_loader_t::WIRE_MAGIC,
		.wire_version		 = font_loader_t::WIRE_VERSION,
		.load				 = font_loader_t::load,
		.create_internals	 = font_loader_t::create_internals,
		.destroy_internals	 = font_loader_t::destroy_internals,
	};
}
