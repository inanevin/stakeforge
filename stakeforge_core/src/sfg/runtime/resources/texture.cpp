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
	bool texture_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		texture_runtime_t* runtime = mem.get<texture_runtime_t>(entry.runtime);

		istream_t stream;
		stream.open(entry.after_header_data.data, entry.after_header_data.size);

		stream >> runtime->channels >> runtime->is_linear >> runtime->mip_count;

		SFG_ASSERT(runtime->mip_count <= MAX_MIPS);

		for (u8 i = 0; i < runtime->mip_count; ++i)
		{
			texture_buffer_t& buf = runtime->mips[i];
			stream >> buf.bpp;
			stream >> buf.size;

			const size_t sz = buf.bpp * buf.size.x * buf.size.y;
			buf.pixels		= stream.get_data_current();
			stream.skip_by(sz);
		}

		return true;
	}

	create_internals_result_e texture_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		 mem	 = ctx.resource_manager.get_memory();
		const texture_runtime_t* runtime = mem.get<texture_runtime_t>(entry.runtime);

		texture_desc_t desc = {};
		desc.texture_format = runtime->is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
		desc.size			= runtime->mips[0].size;
		desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		desc.mip_levels		= runtime->mip_count == 0 ? 1u : runtime->mip_count;
		desc.array_length	= 1;
		desc.samples		= 1;
		desc.debug_name		= "texture";

		render_resources_t::get().enqueue_create_texture(entry.hash, desc);
		return create_internals_result_e::queued;
	}

	resource_ready_result_e texture_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t& mem = ctx.resource_manager.get_memory();

		// done with pixels.
		texture_runtime_t* runtime = mem.get<texture_runtime_t>(entry.runtime);
		for (u8 i = 0; i < runtime->mip_count; i++)
		{
			texture_buffer_t& b = runtime->mips[i];
			b.pixels			= nullptr;
		}

		if (completion.state == resource_state_e::failed)
			return resource_ready_result_e::failed;

		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		internals->texture			   = completion.texture;
		return resource_ready_result_e::ready;
	}

	void texture_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_texture(internals->texture);
		internals->texture = {};
	}

	const resource_type_desc_t texture_resource_desc = {
		.type				 = resource_type_e::texture,
		.runtime_size		 = sizeof(texture_runtime_t),
		.runtime_alignment	 = alignof(texture_runtime_t),
		.internals_size		 = sizeof(texture_internals_t),
		.internals_alignment = alignof(texture_internals_t),
		.wire_magic			 = texture_loader_t::WIRE_MAGIC,
		.wire_version		 = texture_loader_t::WIRE_VERSION,
		.load				 = texture_loader_t::load,
		.create_internals	 = texture_loader_t::create_internals,
		.resource_ready	 = texture_loader_t::resource_ready,
		.destroy_internals	 = texture_loader_t::destroy_internals,
	};
}
