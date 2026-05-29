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

#include "texture.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool texture_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	   = ctx.resource_manager.get_memory();
		texture_runtime_t* runtime = mem.get<texture_runtime_t>(entry.runtime);
		*runtime				   = {};

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

		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);

		SFG_ASSERT(runtime->mip_count > 0);

		*internals				 = texture_internals_t{};
		internals->pending_count = 2;

		texture_desc_t desc = {};
		desc.texture_format = runtime->is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
		desc.size			= runtime->mips[0].size;
		desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		desc.mip_levels		= runtime->mip_count;
		desc.array_length	= 1;
		desc.samples		= 1;
		desc.set_name(mem.get_text(entry.debug_name));

		u32 staging_size = 0;
		for (u8 i = 0; i < runtime->mip_count; ++i)
		{
			const texture_buffer_t& b = runtime->mips[i];
			staging_size			  = gfx_backend::align_texture_size(staging_size);
			staging_size += gfx_backend::get_texture_size(static_cast<u32>(b.size.x), static_cast<u32>(b.size.y), static_cast<u32>(b.bpp));
		}
		staging_size = gfx_backend::align_texture_size(staging_size);

		resource_desc_t staging_desc = {};
		staging_desc.size			 = staging_size;
		staging_desc.flags			 = resource_flags::rf_cpu_visible;
		staging_desc.set_name("texture_upload_staging");

		render_resources_t::get().enqueue_create_texture(entry.hash, desc);
		render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, staging_desc);
		return create_internals_result_e::queued;
	}

	resource_ready_result_e texture_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);

		SFG_ASSERT(internals->pending_count > 0);

		if (completion.state == resource_state_e::failed)
		{
			internals->had_failure = 1;
		}
		else if (completion.kind == render_resource_kind_e::texture)
		{
			internals->texture	 = completion.texture;
			internals->gpu_index = completion.gpu_index;
		}
		else
		{
			SFG_ASSERT(completion.kind == render_resource_kind_e::resource);
			internals->staging = completion.resource;
		}

		internals->pending_count--;
		if (internals->pending_count != 0)
			return resource_ready_result_e::pending;

		if (internals->had_failure)
		{
			render_resources_t::get().enqueue_destroy_texture(internals->texture);
			render_resources_t::get().enqueue_destroy_resource(internals->staging);
			*internals = texture_internals_t{};
			return resource_ready_result_e::failed;
		}

		texture_runtime_t* runtime				 = mem.get<texture_runtime_t>(entry.runtime);
		texture_buffer_t   upload_mips[MAX_MIPS] = {};
		for (u8 i = 0; i < runtime->mip_count; ++i)
		{
			texture_buffer_t& b		 = runtime->mips[i];
			const size_t	  sz	 = static_cast<size_t>(b.bpp) * static_cast<size_t>(b.size.x) * static_cast<size_t>(b.size.y);
			u8*				  pixels = static_cast<u8*>(SFG_MALLOC(sz));
			SFG_MEMCPY(pixels, b.pixels, sz);
			upload_mips[i]		  = b;
			upload_mips[i].pixels = pixels;
			b.pixels			  = nullptr;
		}

		const texture_upload_desc_t upload = {
			.texture	   = internals->texture,
			.staging	   = internals->staging,
			.mips		   = {.data = upload_mips, .size = runtime->mip_count},
			.target_states = resource_state_ps_resource,
			.ownership	   = texture_data_ownership_e::c_free,
		};
		render_resources_t::get().enqueue_texture_upload(upload);

		return resource_ready_result_e::ready;
	}

	void texture_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_texture(internals->texture);
		render_resources_t::get().enqueue_destroy_resource(internals->staging);
		*internals = texture_internals_t{};
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
		.resource_ready		 = texture_loader_t::resource_ready,
		.destroy_internals	 = texture_loader_t::destroy_internals,
	};
}
