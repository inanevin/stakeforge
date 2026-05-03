// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/io/assert.hpp>

namespace sfg
{
	bool texture_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx)
	{
		u32 magic	= 0;
		u32 version = 0;
		stream >> magic >> version;
		if (magic != texture_wire_magic || version != texture_wire_version)
		{
			SFG_ERR("invalid texture binary, magic={0} version={1}", magic, version);
			return false;
		}

		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		texture_data_t*	   data = mem.get<texture_data_t>(entry.cpu_data);
		*data					= texture_data_t{};

		stream >> data->width >> data->height;
		stream >> data->channels >> data->is_linear >> data->mip_count;
		stream >> data->pixels_size;

		SFG_ASSERT(data->mip_count <= texture_max_mips);

		for (u8 i = 0; i < data->mip_count; ++i)
		{
			texture_mip_t& m = data->mips[i];
			stream >> m.offset >> m.size >> m.width >> m.height;
		}

		if (data->pixels_size != 0)
		{
			data->pixels = mem.allocate_bytes(data->pixels_size, 1);
			u8* dst		 = mem.get(data->pixels.head);
			stream.read_to_raw(dst, static_cast<size_t>(data->pixels_size));
		}

		return true;
	}

	bool texture_create_internals(resource_entry_t&, resource_context_t&)
	{
		return true;
	}

	void texture_destroy_internals(resource_entry_t&, resource_context_t&)
	{
	}

	void texture_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void texture_unload_cpu(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		texture_data_t*	   data = mem.get<texture_data_t>(entry.cpu_data);
		if (data->pixels)
			mem.free(data->pixels);
		data->pixels	  = chunk_handle32_t{};
		data->pixels_size = 0;
	}

	const resource_type_desc_t texture_resource_desc = {
		.type				 = resource_type_e::texture,
		.data_size			 = sizeof(texture_data_t),
		.data_alignment		 = alignof(texture_data_t),
		.internals_size		 = sizeof(texture_internals_t),
		.internals_alignment = alignof(texture_internals_t),
		.load				 = texture_load,
		.create_internals	 = texture_create_internals,
		.destroy_internals	 = texture_destroy_internals,
		.unload				 = texture_unload,
		.unload_cpu			 = texture_unload_cpu,
	};
}
