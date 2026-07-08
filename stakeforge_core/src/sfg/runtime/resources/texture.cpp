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
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include "texture_streamer.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	void texture_mip_header_t::serialize(ostream_t& stream) const
	{
		stream << byte_offset << data_size << row_pitch << size << bpp;
	}

	void texture_mip_header_t::deserialize(istream_t& stream)
	{
		stream >> byte_offset >> data_size >> row_pitch >> size >> bpp;
	}

	void texture_header_t::serialize(ostream_t& stream) const
	{
		stream << average_color << texture_format << payload_type << ktx2_compression << size << bpp << mip_count << is_linear << use_streaming;
		for (u8 i = 0; i < texture_loader_t::MAX_MIPS; ++i)
			stream << mips[i];
	}

	void texture_header_t::deserialize(istream_t& stream)
	{
		stream >> average_color >> texture_format >> payload_type >> ktx2_compression >> size >> bpp >> mip_count >> is_linear >> use_streaming;
		for (u8 i = 0; i < texture_loader_t::MAX_MIPS; ++i)
			stream >> mips[i];
	}

	namespace
	{
		static constexpr size_t TEXTURE_MIP_HEADER_WIRE_SIZE = sizeof(u32) * 3 + sizeof(u16) * 2 + sizeof(u8);
		static constexpr size_t TEXTURE_HEADER_WIRE_SIZE	 = sizeof(f32) * 4 + sizeof(u8) * 3 + sizeof(u16) * 2 + sizeof(u8) * 4 + TEXTURE_MIP_HEADER_WIRE_SIZE * texture_loader_t::MAX_MIPS;
		static constexpr u16	TEXTURE_PLACEHOLDER_SIZE	 = 4;

		void free_texture_runtime_mips(texture_runtime_t& runtime)
		{
			for (u8 i = 0; i < runtime.header.mip_count; ++i)
			{
				SFG_FREE(runtime.mips[i].pixels);
				runtime.mips[i].pixels = nullptr;
			}
		}

		u8 float_to_u8(f32 value)
		{
			const f32 clamped = value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
			return static_cast<u8>(clamped * 255.0f + 0.5f);
		}

		texture_desc_t make_texture_desc(const texture_header_t& header, const char* debug_name)
		{
			texture_desc_t desc = {};
			desc.texture_format = header.texture_format;
			desc.size			= header.size;
			desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
			desc.mip_levels		= header.mip_count;
			desc.array_length	= 1;
			desc.samples		= 1;
			desc.set_name(debug_name);
			return desc;
		}

		u32 calculate_texture_staging_size(const texture_header_t& header, const texture_buffer_t* mips)
		{
			u32 staging_size = 0;
			for (u8 i = 0; i < header.mip_count; ++i)
			{
				const texture_buffer_t& b = mips[i];
				staging_size			  = gfx_backend::align_texture_size(staging_size);
				staging_size += gfx_backend::align_texture_size_pitch(b.row_pitch) * format_get_row_count(header.texture_format, b.size.y);
			}
			return gfx_backend::align_texture_size(staging_size);
		}

		resource_desc_t make_texture_staging_desc(const texture_header_t& header, const texture_buffer_t* mips)
		{
			resource_desc_t desc = {};
			desc.size			 = calculate_texture_staging_size(header, mips);
			desc.flags			 = resource_flags::rf_cpu_visible;
			desc.set_name("texture_upload_staging");
			return desc;
		}

		bool enqueue_texture_placeholder_create_and_upload(resource_entry_t& entry, chunk_allocator_t& mem, const texture_header_t& header, texture_internals_t& internals)
		{
			texture_buffer_t placeholder = {};
			placeholder.bpp				 = 4;
			placeholder.size			 = vec2u16_t(TEXTURE_PLACEHOLDER_SIZE, TEXTURE_PLACEHOLDER_SIZE);
			placeholder.row_pitch		 = TEXTURE_PLACEHOLDER_SIZE * placeholder.bpp;
			placeholder.data_size		 = placeholder.row_pitch * TEXTURE_PLACEHOLDER_SIZE;
			placeholder.pixels			 = static_cast<u8*>(SFG_MALLOC(placeholder.data_size));
			if (placeholder.pixels == nullptr)
			{
				SFG_ERR("failed to allocate texture placeholder pixels: {0}", entry.hash);
				return false;
			}

			const u8 color[4] = {
				float_to_u8(header.average_color.x),
				float_to_u8(header.average_color.y),
				float_to_u8(header.average_color.z),
				float_to_u8(header.average_color.w),
			};
			for (u32 i = 0; i < TEXTURE_PLACEHOLDER_SIZE * TEXTURE_PLACEHOLDER_SIZE; ++i)
				SFG_MEMCPY(placeholder.pixels + static_cast<size_t>(i) * placeholder.bpp, color, placeholder.bpp);

			texture_header_t placeholder_header = header;
			placeholder_header.texture_format	= header.is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
			placeholder_header.size				= placeholder.size;
			placeholder_header.bpp				= placeholder.bpp;
			placeholder_header.mip_count		= 1;

			const texture_desc_t  desc		   = make_texture_desc(placeholder_header, mem.get_text(entry.debug_name));
			const resource_desc_t staging_desc = make_texture_staging_desc(placeholder_header, &placeholder);

			resource_manager_t::get().bump_render_pending(entry, 2);
			internals.texture = render_resources_t::get().enqueue_create_texture(entry.hash, desc);
			internals.staging = render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, staging_desc);

			render_resources_t::get().enqueue_texture_upload({
				.mips			   = {.data = &placeholder, .size = 1},
				.texture		   = internals.texture,
				.staging		   = internals.staging,
				.target_states	   = resource_state_ps_resource,
				.destination_slice = 0,
				.ownership		   = texture_data_ownership_e::c_free,
			});

			return true;
		}

		bool enqueue_texture_create_and_upload(resource_entry_t& entry, chunk_allocator_t& mem, const texture_header_t& header, texture_buffer_t* mips, texture_internals_t& internals)
		{
			const texture_desc_t  desc		   = make_texture_desc(header, mem.get_text(entry.debug_name));
			const resource_desc_t staging_desc = make_texture_staging_desc(header, mips);

			resource_manager_t::get().bump_render_pending(entry, 2);
			internals.texture = render_resources_t::get().enqueue_create_texture(entry.hash, desc);
			internals.staging = render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, staging_desc);

			render_resources_t::get().enqueue_texture_upload({
				.mips			   = {.data = mips, .size = header.mip_count},
				.texture		   = internals.texture,
				.staging		   = internals.staging,
				.target_states	   = resource_state_ps_resource,
				.destination_slice = 0,
				.ownership		   = texture_data_ownership_e::c_free,
			});

			return true;
		}

	}

	bool texture_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t header_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), TEXTURE_HEADER_WIRE_SIZE, header_stream))
		{
			SFG_ERR("failed to read texture header: {0}", entry.hash);
			return false;
		}

		istream_t stream;
		stream.open(header_stream.get_raw(), header_stream.get_size());

		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_runtime_t*	 runtime   = mem.get<texture_runtime_t>(entry.runtime);
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);
		*runtime					   = {};
		*internals					   = {};

		stream >> runtime->header;
		SFG_ASSERT(runtime->header.mip_count <= MAX_MIPS);

		if (runtime->header.use_streaming == 0)
		{
			texture_stream_result_t result = texture_streamer_t::load_result(entry.hash, entry.source_ticks, rfs);
			if (!result.success)
			{
				texture_streamer_t::release_result(result);
				return false;
			}

			if (!enqueue_texture_create_and_upload(entry, mem, result.header, result.mips, *internals))
			{
				texture_streamer_t::release_result(result);
				return false;
			}

			runtime->header	   = result.header;
			runtime->residency = texture_residency_e::resident;
			for (u8 i = 0; i < result.header.mip_count; ++i)
				result.mips[i].pixels = nullptr;
			return true;
		}

		if (!enqueue_texture_placeholder_create_and_upload(entry, mem, runtime->header, *internals))
			return false;

		runtime->residency = texture_residency_e::streaming;
		ctx.resource_manager.get_texture_streamer().enqueue(entry, rfs);
		return true;
	}

	void texture_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 mem	   = ctx.resource_manager.get_memory();
		texture_runtime_t*	 runtime   = mem.get<texture_runtime_t>(entry.runtime);
		texture_internals_t* internals = mem.get<texture_internals_t>(entry.internals);

		free_texture_runtime_mips(*runtime);
		render_resources_t::get().enqueue_destroy_texture(internals->texture);
		render_resources_t::get().enqueue_destroy_resource(internals->staging);
		*internals = {};
	}

	const resource_type_desc_t texture_resource_desc = {
		.type				 = resource_type_e::texture,
		.runtime_size		 = sizeof(texture_runtime_t),
		.runtime_alignment	 = alignof(texture_runtime_t),
		.internals_size		 = sizeof(texture_internals_t),
		.internals_alignment = alignof(texture_internals_t),
		.wire_magic			 = texture_loader_t::WIRE_MAGIC,
		.wire_version		 = texture_loader_t::WIRE_VERSION,
		.use_render_pending	 = true,
		.load				 = texture_loader_t::load,
		.unload				 = texture_loader_t::unload,
	};

}
