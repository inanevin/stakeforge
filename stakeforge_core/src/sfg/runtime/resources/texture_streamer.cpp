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

#include "texture_streamer.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/stb/stb_image.h>
#include <cstdint>
#include <ktx.h>

namespace sfg
{
#define SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM  37
#define SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB	  43
#define SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK 145
#define SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK  146

	namespace
	{
		u16 get_mip_size(u32 base_size, u8 mip)
		{
			const u32 size = base_size >> mip;
			return static_cast<u16>(size == 0 ? 1 : size);
		}

		format_e get_format_from_ktx(ktx_uint32_t vk_format)
		{
			switch (vk_format)
			{
			case SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK:
				return format_e::bc7_block_srgb;
			case SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK:
				return format_e::bc7_block_unorm;
			case SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB:
				return format_e::r8g8b8a8_srgb;
			case SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM:
				return format_e::r8g8b8a8_unorm;
			default:
				return format_e::undefined;
			}
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

		void release_stream_result(texture_stream_result_t& result)
		{
			for (u8 i = 0; i < result.header.mip_count; ++i)
			{
				SFG_FREE(result.mips[i].pixels);
				result.mips[i].pixels = nullptr;
			}
		}

		bool copy_texture_mip_to_rgba(texture_buffer_t& buf, const u8* src, u8 src_bpp)
		{
			const u32 pixel_count = static_cast<u32>(buf.size.x) * static_cast<u32>(buf.size.y);
			buf.bpp				  = 4;
			buf.row_pitch		  = static_cast<u32>(buf.size.x) * 4;
			buf.data_size		  = pixel_count * 4;
			buf.pixels			  = static_cast<u8*>(SFG_MALLOC(buf.data_size));
			if (buf.pixels == nullptr)
			{
				SFG_ERR("failed to allocate texture mip pixels");
				return false;
			}

			if (src_bpp == 4)
			{
				SFG_MEMCPY(buf.pixels, src, buf.data_size);
				return true;
			}

			u8* dst = buf.pixels;
			for (u32 i = 0; i < pixel_count; ++i)
			{
				const u8* p = src + static_cast<size_t>(i) * src_bpp;
				if (src_bpp == 1)
				{
					dst[0] = p[0];
					dst[1] = p[0];
					dst[2] = p[0];
					dst[3] = 255;
				}
				else if (src_bpp == 2)
				{
					dst[0] = p[0];
					dst[1] = p[0];
					dst[2] = p[0];
					dst[3] = p[1];
				}
				else
				{
					dst[0] = p[0];
					dst[1] = p[1];
					dst[2] = p[2];
					dst[3] = 255;
				}
				dst += 4;
			}

			return true;
		}

		texture_stream_result_t load_texture_stream_result(sid_t hash, u64 source_ticks, resource_file_system_t& rfs)
		{
			texture_stream_result_t result = {};
			result.hash					   = hash;
			result.source_ticks			   = source_ticks;

			ostream_t file_stream;
			if (!rfs.read_resource(hash, sizeof(resource_header_t), 0, file_stream))
			{
				SFG_ERR("failed to read texture resource: {0}", hash);
				return result;
			}

			istream_t stream;
			stream.open(file_stream.get_raw(), file_stream.get_size());
			stream >> result.header;
			SFG_ASSERT(result.header.mip_count <= texture_loader_t::MAX_MIPS);

			if (result.header.payload_type == texture_payload_type_e::uncompressed)
			{
				u32 blob_size = 0;
				stream >> blob_size;
				istream_t compressed;
				compressed.open(stream.get_data_current(), blob_size);
				istream_t payload = compressor_t::decompress(compressed);
				stream.skip_by(blob_size);
				if (payload.empty())
				{
					SFG_ERR("failed to decompress texture payload: {0}", hash);
					return result;
				}

				for (u8 i = 0; i < result.header.mip_count; ++i)
				{
					const texture_mip_header_t& mip = result.header.mips[i];
					texture_buffer_t&			buf = result.mips[i];
					buf.size						= mip.size;

					if (!copy_texture_mip_to_rgba(buf, payload.get_raw() + mip.byte_offset, mip.bpp))
					{
						SFG_ERR("failed to prepare uncompressed texture mip: {0}", hash);
						release_stream_result(result);
						return result;
					}
					result.header.mips[i] = {
						.byte_offset = 0,
						.data_size	 = buf.data_size,
						.row_pitch	 = buf.row_pitch,
						.size		 = buf.size,
						.bpp		 = buf.bpp,
					};
				}
				result.header.bpp = 4;
				result.success	  = true;
				return result;
			}

			if (result.header.payload_type == texture_payload_type_e::png)
			{
				const size_t payload_offset = stream.tellg();
				for (u8 i = 0; i < result.header.mip_count; ++i)
				{
					const texture_mip_header_t& mip = result.header.mips[i];

					int		 decoded_width	  = 0;
					int		 decoded_height	  = 0;
					int		 decoded_channels = 0;
					stbi_uc* decoded		  = stbi_load_from_memory(file_stream.get_raw() + payload_offset + mip.byte_offset, static_cast<int>(mip.data_size), &decoded_width, &decoded_height, &decoded_channels, 4);

					if (decoded == nullptr)
					{
						SFG_ERR("failed to decode PNG texture mip: {0}", hash);
						release_stream_result(result);
						return result;
					}

					texture_buffer_t& buf = result.mips[i];
					buf.bpp				  = 4;
					buf.size			  = mip.size;
					buf.row_pitch		  = static_cast<u32>(mip.size.x) * 4;
					buf.data_size		  = buf.row_pitch * static_cast<u32>(mip.size.y);
					buf.pixels			  = static_cast<u8*>(SFG_MALLOC(buf.data_size));
					if (buf.pixels == nullptr)
					{
						SFG_ERR("failed to allocate PNG texture mip pixels: {0}", hash);
						stbi_image_free(decoded);
						release_stream_result(result);
						return result;
					}
					SFG_MEMCPY(buf.pixels, decoded, buf.data_size);
					stbi_image_free(decoded);
					result.header.mips[i] = {
						.byte_offset = 0,
						.data_size	 = buf.data_size,
						.row_pitch	 = buf.row_pitch,
						.size		 = buf.size,
						.bpp		 = buf.bpp,
					};
				}
				result.header.bpp = 4;
				result.success	  = true;
				return result;
			}

			const texture_mip_header_t& ktx_mip		   = result.header.mips[0];
			const size_t				payload_offset = stream.tellg();
			ktxTexture2*				ktx_texture	   = nullptr;
			KTX_error_code				ktx_result	   = ktxTexture2_CreateFromMemory(file_stream.get_raw() + payload_offset + ktx_mip.byte_offset, ktx_mip.data_size, 0, &ktx_texture);

			if (ktx_result == KTX_SUCCESS && result.header.ktx2_compression == texture_ktx2_compression_e::fastest)
				SFG_ASSERT(ktx_texture->supercompressionScheme != KTX_SS_ZSTD);

			if (ktx_result == KTX_SUCCESS)
				ktx_result = ktxTexture2_LoadImageData(ktx_texture, nullptr, 0);

			if (ktx_result == KTX_SUCCESS && ktxTexture2_NeedsTranscoding(ktx_texture))
				ktx_result = ktxTexture2_TranscodeBasis(ktx_texture, KTX_TTF_BC7_RGBA, KTX_TF_HIGH_QUALITY);

			if (ktx_result != KTX_SUCCESS)
			{
				SFG_ERR("failed to transcode KTX2 texture: {0}", ktxErrorString(ktx_result));
				if (ktx_texture != nullptr)
					ktxTexture2_Destroy(ktx_texture);
				return result;
			}

			result.header.texture_format = get_format_from_ktx(ktx_texture->vkFormat);
			if (result.header.texture_format == format_e::undefined)
			{
				SFG_ERR("unsupported KTX2 transcode format");
				ktxTexture2_Destroy(ktx_texture);
				return result;
			}

			result.header.mip_count = static_cast<u8>(ktx_texture->numLevels);
			if (result.header.mip_count > texture_loader_t::MAX_MIPS)
			{
				SFG_ERR("texture has too many KTX2 mip levels");
				ktxTexture2_Destroy(ktx_texture);
				return result;
			}
			result.header.size = vec2u16_t(static_cast<u16>(ktx_texture->baseWidth), static_cast<u16>(ktx_texture->baseHeight));
			result.header.bpp  = format_is_block_compressed(result.header.texture_format) ? 16 : format_get_bpp(result.header.texture_format);

			for (u8 i = 0; i < result.header.mip_count; ++i)
			{
				ktx_size_t offset = 0;
				ktx_result		  = ktxTexture_GetImageOffset(ktxTexture(ktx_texture), i, 0, 0, &offset);
				if (ktx_result != KTX_SUCCESS)
				{
					SFG_ERR("failed to get KTX2 texture image offset: {0}", ktxErrorString(ktx_result));
					release_stream_result(result);
					ktxTexture2_Destroy(ktx_texture);
					return result;
				}

				const ktx_size_t image_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), i);
				if (image_size > UINT32_MAX)
				{
					SFG_ERR("texture KTX2 image is too large");
					release_stream_result(result);
					ktxTexture2_Destroy(ktx_texture);
					return result;
				}

				texture_buffer_t& buf = result.mips[i];
				buf.data_size		  = static_cast<u32>(image_size);
				buf.row_pitch		  = ktxTexture_GetRowPitch(ktxTexture(ktx_texture), i);
				buf.size			  = vec2u16_t(get_mip_size(ktx_texture->baseWidth, i), get_mip_size(ktx_texture->baseHeight, i));
				buf.bpp				  = result.header.bpp;
				buf.pixels			  = static_cast<u8*>(SFG_MALLOC(buf.data_size));
				if (buf.pixels == nullptr)
				{
					SFG_ERR("failed to allocate KTX2 texture mip pixels: {0}", hash);
					release_stream_result(result);
					ktxTexture2_Destroy(ktx_texture);
					return result;
				}
				SFG_MEMCPY(buf.pixels, ktxTexture_GetData(ktxTexture(ktx_texture)) + offset, buf.data_size);
				result.header.mips[i] = {
					.byte_offset = static_cast<u32>(offset),
					.data_size	 = buf.data_size,
					.row_pitch	 = buf.row_pitch,
					.size		 = buf.size,
					.bpp		 = buf.bpp,
				};
			}

			ktxTexture2_Destroy(ktx_texture);
			result.success = true;
			return result;
		}
	}

	void texture_streamer_t::enqueue(resource_entry_t& entry, resource_file_system_t& rfs)
	{
		SFG_ASSERT(job_system_t::get().is_initialized());
		const sid_t				hash		 = entry.hash;
		const u64				source_ticks = entry.source_ticks;
		resource_file_system_t* rfs_ptr		 = &rfs;
		texture_streamer_t*		streamer	 = this;
		job_system_t::get().silent_async([hash, source_ticks, rfs_ptr, streamer]() mutable { streamer->_results.enqueue(load_result(hash, source_ticks, *rfs_ptr)); });
	}

	texture_stream_result_t texture_streamer_t::load_result(sid_t hash, u64 source_ticks, resource_file_system_t& rfs)
	{
		return load_texture_stream_result(hash, source_ticks, rfs);
	}

	void texture_streamer_t::release_result(texture_stream_result_t& result)
	{
		release_stream_result(result);
	}

	void texture_streamer_t::flush_completed(resource_manager_t& resource_manager)
	{
		texture_stream_result_t result = {};
		while (_results.try_dequeue(result))
		{
			const resource_entry_t* entry = resource_manager.find_entry(result.hash);
			if (entry == nullptr || entry->type != resource_type_e::texture || entry->source_ticks != result.source_ticks || entry->ref_count == 0)
			{
				release_stream_result(result);
				continue;
			}

			chunk_allocator_t&	 mem	   = resource_manager.get_memory();
			texture_runtime_t*	 runtime   = mem.get<texture_runtime_t>(entry->runtime);
			texture_internals_t* internals = mem.get<texture_internals_t>(entry->internals);

			if (!result.success)
			{
				runtime->residency = texture_residency_e::failed;
				continue;
			}

			if (entry->state == resource_state_e::pending_render)
			{
				_results.enqueue(std::move(result));
				break;
			}

			const render_resource_handle_t old_staging	= internals->staging;
			const resource_desc_t		   staging_desc = make_texture_staging_desc(result.header, result.mips);
			internals->staging							= render_resources_t::get().enqueue_create_resource(0, entry->type, staging_desc);

			const texture_desc_t texture_desc = make_texture_desc(result.header, mem.get_text(entry->debug_name));
			render_resources_t::get().enqueue_replace_texture({
				.mips		   = {.data = result.mips, .size = result.header.mip_count},
				.texture	   = internals->texture,
				.staging	   = internals->staging,
				.old_staging   = old_staging,
				.texture_desc  = texture_desc,
				.target_states = resource_state_ps_resource,
				.ownership	   = texture_data_ownership_e::c_free,
			});

			runtime->header	   = result.header;
			runtime->residency = texture_residency_e::resident;
			for (u8 i = 0; i < result.header.mip_count; ++i)
				result.mips[i].pixels = nullptr;
		}
	}

#undef SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM
#undef SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB
#undef SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK
#undef SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK
}
