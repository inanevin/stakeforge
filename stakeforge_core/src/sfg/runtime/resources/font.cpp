// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool font_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	 = ctx.resource_manager.get_memory();
		u8*				   bytes = mem.get(entry.runtime.head);

		istream_t stream;
		stream.open(bytes, entry.runtime.size);

		font_runtime_t local = {};
		stream >> local.ascent >> local.descent >> local.line_gap;
		stream >> local.size >> local.scale >> local.kind;

		for (u32 i = 0; i < 128; i++)
		{
			font_runtime_glyph_t& g = local.glyph_info[i];
			stream >> g.pixel_offset >> g.pixel_size;
			stream >> g.width >> g.height >> g.advance_x >> g.left_bearing;
			stream >> g.x_offset >> g.y_offset;
			for (i32 k = 0; k < 128; k++)
				stream >> g.kern_advance[k];
		}

		*mem.get<font_runtime_t>(entry.runtime) = local;
		return true;
	}

	create_internals_result_e font_loader_t::create_internals(resource_entry_t&, resource_context_t&)
	{
		return create_internals_result_e::ready;
	}

	void font_loader_t::destroy_internals(resource_entry_t&, resource_context_t&)
	{
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
