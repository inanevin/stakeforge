// Copyright (c) 2025 Inan Evin

#include "texture.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool texture_load(resource_entry_t& entry, istream_t& stream, resource_context_t& ctx)
	{
		u32 magic		 = 0;
		u32 version		 = 0;
		u32 payload_size = 0;
		stream >> magic >> version >> payload_size;
		if (magic != texture_wire_magic || version != texture_wire_version)
		{
			SFG_ERR("invalid texture binary, magic={0} version={1}", magic, version);
			return false;
		}

		chunk_allocator_t& mem	= ctx.resource_manager.get_memory();
		texture_data_t*	   data = mem.get<texture_data_t>(entry.cpu_data);
		*data					= texture_data_t{};

		data->pixels	  = entry.payload;
		data->pixels_size = payload_size;

		stream >> data->width >> data->height;
		stream >> data->channels >> data->is_linear >> data->mip_count;

		SFG_ASSERT(data->mip_count <= texture_max_mips);

		for (u8 i = 0; i < data->mip_count; ++i)
		{
			texture_mip_t& m = data->mips[i];
			stream >> m.offset >> m.size >> m.width >> m.height;
		}

		if (payload_size != 0)
		{
			u8* dst = mem.get(entry.payload.head);
			stream.read_to_raw(dst, static_cast<size_t>(payload_size));
		}

		return true;
	}

	bool texture_create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem  = ctx.resource_manager.get_memory();
		const texture_data_t* data = mem.get<texture_data_t>(entry.cpu_data);

		texture_desc_t desc = {};
		desc.texture_format = data->is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
		desc.size			= {static_cast<u16>(data->width), static_cast<u16>(data->height)};
		desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		desc.mip_levels		= data->mip_count == 0 ? 1u : data->mip_count;
		desc.array_length	= 1;
		desc.samples		= 1;
		desc.debug_name		= "texture";

		render_resources_t::get().enqueue_create_texture(entry.hash, desc);
		return true;
	}

	bool texture_complete_internals(resource_entry_t& entry, resource_context_t& ctx, const resource_internals_completion_t& completion)
	{
		SFG_ASSERT(completion.data != nullptr);
		SFG_ASSERT(completion.size == sizeof(gfx_texture_handle));

		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		internals->texture			   = *static_cast<const gfx_texture_handle*>(completion.data);
		return !internals->texture.is_null();
	}

	void texture_destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_texture(internals->texture);
		internals->texture = {};
	}

	void texture_unload(resource_entry_t&, resource_context_t&)
	{
	}

	void texture_unload_cpu(resource_entry_t&, resource_context_t&)
	{
	}

	const resource_type_desc_t texture_resource_desc = {
		.type				 = resource_type_e::texture,
		.data_size			 = sizeof(texture_data_t),
		.data_alignment		 = alignof(texture_data_t),
		.internals_size		 = sizeof(texture_internals_t),
		.internals_alignment = alignof(texture_internals_t),
		.load				 = texture_load,
		.create_internals	 = texture_create_internals,
		.complete_internals	 = texture_complete_internals,
		.destroy_internals	 = texture_destroy_internals,
		.unload				 = texture_unload,
		.unload_cpu			 = texture_unload_cpu,
	};
}
