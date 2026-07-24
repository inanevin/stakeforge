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

#include "cubemap.hpp"

#include "cubemap_data.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	namespace
	{
		void load_texture_block(istream_t& stream, cubemap_texture_block_t& block)
		{
			stream >> block.format;
			stream >> block.size;
			stream >> block.mip_count;

			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					texture_buffer_t& buffer = block.buffers[face * cubemap_loader_t::MAX_MIPS + mip];
					stream >> buffer.size;
					stream >> buffer.row_pitch;
					stream >> buffer.data_size;
					buffer.bpp	  = format_get_bpp(block.format);
					buffer.pixels = stream.get_data_current();
					stream.skip_by(buffer.data_size);
				}
			}
		}

		u32 get_face_staging_size(const cubemap_texture_block_t& block)
		{
			u32 staging_size = 0;

			for (u8 mip = 0; mip < block.mip_count; ++mip)
			{
				const texture_buffer_t& buffer = block.buffers[mip];
				staging_size				   = gfx_backend::align_texture_size(staging_size);
				staging_size += gfx_backend::align_texture_size_pitch(buffer.row_pitch) * format_get_row_count(block.format, buffer.size.y);
			}

			return gfx_backend::align_texture_size(staging_size);
		}

		texture_desc_t make_texture_desc(const cubemap_texture_block_t& block)
		{
			texture_desc_t desc		  = {};
			desc.texture_format		  = block.format;
			desc.size				  = block.size;
			desc.flags				  = texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d | texture_flags::tf_cubemap;
			desc.mip_levels			  = block.mip_count;
			desc.array_length		  = cubemap_loader_t::FACE_COUNT;
			desc.samples			  = 1;
			desc.views[0].mip_count	  = 0;
			desc.views[0].level_count = 0;
			desc.views[0].is_cubemap  = 1;
			desc.set_name("cubemap");
			return desc;
		}

		resource_desc_t make_staging_desc(const cubemap_texture_block_t& block)
		{
			resource_desc_t desc = {};
			desc.size			 = get_face_staging_size(block);
			desc.flags			 = resource_flags::rf_cpu_visible;
			desc.set_name("cubemap_upload_staging");
			return desc;
		}

		bool enqueue_upload(const cubemap_texture_block_t& block, const cubemap_internals_t& internals)
		{
			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			{
				texture_buffer_t upload_mips[cubemap_loader_t::MAX_MIPS] = {};

				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					const texture_buffer_t& src = block.buffers[face * cubemap_loader_t::MAX_MIPS + mip];
					texture_buffer_t&		dst = upload_mips[mip];
					dst							= src;
					dst.pixels					= static_cast<u8*>(SFG_MALLOC(src.data_size));

					if (dst.pixels == nullptr)
					{
						for (u8 allocated_mip = 0; allocated_mip < mip; ++allocated_mip)
							SFG_FREE(upload_mips[allocated_mip].pixels);

						return false;
					}

					SFG_MEMCPY(dst.pixels, src.pixels, src.data_size);
				}

				render_resources_t::get().enqueue_texture_upload({
					.mips			   = {.data = upload_mips, .size = block.mip_count},
					.texture		   = internals.texture,
					.staging		   = internals.staging[face],
					.target_states	   = resource_state_ps_resource,
					.destination_slice = face,
					.ownership		   = texture_data_ownership_e::c_free,
				});
			}

			return true;
		}

		void enqueue_destroy(const cubemap_internals_t& internals)
		{
			render_resources_t::get().enqueue_destroy_texture(internals.texture);

			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
				render_resources_t::get().enqueue_destroy_resource(internals.staging[face]);
		}
	}

	bool cubemap_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read cubemap resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};
		stream.open(file_stream.get_raw(), file_stream.get_size());

		istream_t payload = compressor_t::decompress(stream);

		if (payload.empty())
		{
			SFG_ERR("failed to decompress cubemap payload: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		cubemap_runtime_t*	 runtime   = mem.get<cubemap_runtime_t>(entry.runtime);
		cubemap_internals_t* internals = mem.get<cubemap_internals_t>(entry.internals);

		*runtime   = {};
		*internals = {};

		cubemap_texture_block_t texture = {};
		load_texture_block(payload, texture);

		render_resources_t& render_resources = render_resources_t::get();
		internals->texture					 = render_resources.enqueue_create_texture(make_texture_desc(texture));

		const resource_desc_t staging_desc = make_staging_desc(texture);

		for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			internals->staging[face] = render_resources.enqueue_create_resource(staging_desc);

		if (!enqueue_upload(texture, *internals))
		{
			SFG_ERR("failed to allocate cubemap upload data: {0}", entry.hash);
			enqueue_destroy(*internals);
			*internals = {};
			return false;
		}

		runtime->format	   = texture.format;
		runtime->size	   = texture.size;
		runtime->mip_count = texture.mip_count;

		return true;
	}

	void cubemap_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		cubemap_internals_t* internals = mem.get<cubemap_internals_t>(entry.internals);

		enqueue_destroy(*internals);
		*internals = {};
	}

	const resource_type_desc_t cubemap_resource_desc = {
		.type				 = resource_type_e::cubemap,
		.runtime_size		 = sizeof(cubemap_runtime_t),
		.runtime_alignment	 = alignof(cubemap_runtime_t),
		.internals_size		 = sizeof(cubemap_internals_t),
		.internals_alignment = alignof(cubemap_internals_t),
		.wire_magic			 = cubemap_loader_t::WIRE_MAGIC,
		.wire_version		 = cubemap_loader_t::WIRE_VERSION,
		.load				 = cubemap_loader_t::load,
		.unload				 = cubemap_loader_t::unload,
	};
}
