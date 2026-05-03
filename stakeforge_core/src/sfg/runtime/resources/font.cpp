// Copyright (c) 2025 Inan Evin

#include "font.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool font_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx)
	{
		u32 magic		 = 0;
		u32 version		 = 0;
		u32 payload_size = 0;
		stream >> magic >> version >> payload_size;
		if (magic != font_wire_magic || version != font_wire_version)
		{
			SFG_ERR("invalid font binary, magic={0} version={1}", magic, version);
			return false;
		}

		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		font_data_t*	   data = mem.get<font_data_t>(entry.cpu_data);

		data->pixels	  = entry.payload;
		data->pixels_size = payload_size;

		stream >> data->ascent >> data->descent >> data->line_gap;
		stream >> data->size >> data->scale >> data->kind;

		for (u32 i = 0; i < 128; i++)
		{
			font_glyph_t& g = data->glyph_info[i];
			stream >> g.pixel_offset >> g.pixel_size;
			stream >> g.width >> g.height >> g.advance_x >> g.left_bearing;
			stream >> g.x_offset >> g.y_offset;
			for (i32 k = 0; k < 128; k++)
				stream >> g.kern_advance[k];
		}

		if (payload_size != 0)
		{
			u8* dst = mem.get(entry.payload.head);
			stream.read_to_raw(dst, static_cast<size_t>(payload_size));
		}

		return true;
	}

	bool font_create_internals(resource_entry_t&, resource_context_t&)
	{
		return true;
	}

	void font_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void font_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void font_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t font_resource_desc = {
		.type				 = resource_type_e::font,
		.data_size			 = sizeof(font_data_t),
		.data_alignment		 = alignof(font_data_t),
		.internals_size		 = sizeof(font_internals_t),
		.internals_alignment = alignof(font_internals_t),
		.load				 = font_load,
		.create_internals	 = font_create_internals,
		.destroy_internals	 = font_destroy_internals,
		.unload				 = font_unload,
		.unload_cpu			 = font_unload_cpu,
	};
}
