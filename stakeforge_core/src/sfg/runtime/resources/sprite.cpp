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

#include "sprite.hpp"
#include "ktx2_util.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/vendor/stb/stb_image.h>

namespace sfg
{
	void sprite_header_t::serialize(ostream_t& stream) const
	{
		stream << size << cell_size << padding << data_size << row_count << column_count << payload_type << ktx2_compression;
	}

	void sprite_header_t::deserialize(istream_t& stream)
	{
		stream >> size >> cell_size >> padding >> data_size >> row_count >> column_count >> payload_type >> ktx2_compression;
	}

	bool sprite_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t payload_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, payload_stream))
		{
			SFG_ERR("failed to read sprite resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(payload_stream.get_raw(), payload_stream.get_size());

		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		sprite_runtime_t*	runtime	  = mem.get<sprite_runtime_t>(entry.runtime);
		sprite_internals_t* internals = mem.get<sprite_internals_t>(entry.internals);
		*runtime					  = {};
		*internals					  = {};

		stream >> runtime->header;

		texture_buffer_t mip	= {};
		format_e		 format = format_e::undefined;

		if (runtime->header.payload_type == sprite_payload_type_e::png)
		{
			int		 decoded_width	  = 0;
			int		 decoded_height	  = 0;
			int		 decoded_channels = 0;
			stbi_uc* decoded		  = stbi_load_from_memory(stream.get_data_current(), static_cast<int>(runtime->header.data_size), &decoded_width, &decoded_height, &decoded_channels, 4);

			if (decoded == nullptr)
			{
				SFG_ERR("failed to decode PNG sprite: {0}", entry.hash);
				return false;
			}

			mip.bpp		  = 4;
			mip.size	  = runtime->header.size;
			mip.row_pitch = static_cast<u32>(mip.size.x) * 4;
			mip.data_size = mip.row_pitch * static_cast<u32>(mip.size.y);
			mip.pixels	  = static_cast<u8*>(SFG_MALLOC(mip.data_size));

			if (mip.pixels == nullptr)
			{
				SFG_ERR("failed to allocate PNG sprite pixels: {0}", entry.hash);
				stbi_image_free(decoded);
				return false;
			}

			SFG_MEMCPY(mip.pixels, decoded, mip.data_size);
			stbi_image_free(decoded);
			format = format_e::r8g8b8a8_srgb;
		}
		else
		{
			const span_t<const u8> ktx_data = {
				.data = stream.get_data_current(),
				.size = runtime->header.data_size,
			};
			ktx2_image_desc_t ktx_desc = {};

			if (!ktx2_util_t::decode_uastc(ktx_data, runtime->header.ktx2_compression, entry.hash, &mip, 1, ktx_desc))
				return false;

			SFG_ASSERT(ktx_desc.mip_count == 1);
			SFG_ASSERT(ktx_desc.size == runtime->header.size);
			format = ktx_desc.format;
		}

		texture_desc_t texture_desc = {};
		texture_desc.texture_format = format;
		texture_desc.size			= runtime->header.size;
		texture_desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		texture_desc.mip_levels		= 1;
		texture_desc.array_length	= 1;
		texture_desc.samples		= 1;
		texture_desc.set_name(mem.get_text(entry.debug_name));

		resource_desc_t staging_desc = {};
		staging_desc.size			 = gfx_backend::align_texture_size(gfx_backend::align_texture_size_pitch(mip.row_pitch) * format_get_row_count(format, mip.size.y));
		staging_desc.flags			 = resource_flags::rf_cpu_visible;
		staging_desc.set_name("sprite_upload_staging");

		internals->texture = render_resources_t::get().enqueue_create_texture(texture_desc);
		internals->staging = render_resources_t::get().enqueue_create_resource(staging_desc);

		render_resources_t::get().enqueue_texture_upload({
			.mips			   = {.data = &mip, .size = 1},
			.texture		   = internals->texture,
			.staging		   = internals->staging,
			.target_states	   = resource_state_ps_resource,
			.destination_slice = 0,
			.ownership		   = texture_data_ownership_e::c_free,
		});

		mip.pixels = nullptr;

		const vec2f_t inv_size = {
			1.0f / static_cast<f32>(runtime->header.size.x),
			1.0f / static_cast<f32>(runtime->header.size.y),
		};
		runtime->uv_size = {
			static_cast<f32>(runtime->header.cell_size.x) * inv_size.x,
			static_cast<f32>(runtime->header.cell_size.y) * inv_size.y,
		};
		runtime->uv_stride = {
			static_cast<f32>(runtime->header.cell_size.x + runtime->header.padding.x) * inv_size.x,
			static_cast<f32>(runtime->header.cell_size.y + runtime->header.padding.y) * inv_size.y,
		};

		return true;
	}

	void sprite_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		sprite_internals_t* internals = mem.get<sprite_internals_t>(entry.internals);

		render_resources_t::get().enqueue_destroy_texture(internals->texture);
		render_resources_t::get().enqueue_destroy_resource(internals->staging);
		*internals = {};
	}

	const resource_type_desc_t sprite_resource_desc = {
		.type				 = resource_type_e::sprite,
		.runtime_size		 = sizeof(sprite_runtime_t),
		.runtime_alignment	 = alignof(sprite_runtime_t),
		.internals_size		 = sizeof(sprite_internals_t),
		.internals_alignment = alignof(sprite_internals_t),
		.wire_magic			 = sprite_loader_t::WIRE_MAGIC,
		.wire_version		 = sprite_loader_t::WIRE_VERSION,
		.load				 = sprite_loader_t::load,
		.unload				 = sprite_loader_t::unload,
	};
}
