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

#include "texture_cook.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "texture.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/stb/stb_image_write.h>
#include <cstdint>
#include <cstdlib>
#include <ktx.h>

namespace sfg
{
#define SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM   37
#define SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB	   43
#define SFG_KTX_ZSTD_COMPRESSION_FASTER	   1
#define SFG_KTX_ZSTD_COMPRESSION_DEFAULT   3
#define SFG_KTX_ZSTD_COMPRESSION_HIGH	   5
#define TEXTURE_AVERAGE_COLOR_SAMPLE_COUNT 1024

	namespace
	{
		vec4f_t sample_average_color(const texture_buffer_t& buffer)
		{
			const u32 pixel_count  = static_cast<u32>(buffer.size.x) * static_cast<u32>(buffer.size.y);
			const u32 sample_count = pixel_count < TEXTURE_AVERAGE_COLOR_SAMPLE_COUNT ? pixel_count : TEXTURE_AVERAGE_COLOR_SAMPLE_COUNT;
			u64		  state		   = (static_cast<u64>(buffer.size.x) << 32) ^ (static_cast<u64>(buffer.size.y) << 16) ^ buffer.bpp ^ 0x9e3779b97f4a7c15ull;
			vec4f_t	  sum		   = vec4f_t::zero;

			for (u32 i = 0; i < sample_count; ++i)
			{
				state				  = state * 6364136223846793005ull + 1442695040888963407ull;
				const u32 pixel_index = static_cast<u32>((state >> 32) % pixel_count);
				const u8* pixel		  = buffer.pixels + static_cast<size_t>(pixel_index) * buffer.bpp;
				const f32 r			  = static_cast<f32>(pixel[0]) / 255.0f;
				const f32 g			  = buffer.bpp > 2 ? static_cast<f32>(pixel[1]) / 255.0f : r;
				const f32 b			  = buffer.bpp > 2 ? static_cast<f32>(pixel[2]) / 255.0f : r;
				const f32 a			  = buffer.bpp == 2 ? static_cast<f32>(pixel[1]) / 255.0f : buffer.bpp > 3 ? static_cast<f32>(pixel[3]) / 255.0f : 1.0f;
				sum += {r, g, b, a};
			}

			return sum / static_cast<f32>(sample_count);
		}

		void free_texture_buffers(texture_buffer_t* buffers, u8 levels, bool base_from_image_util)
		{
			if (base_from_image_util)
				image_util_t::free(buffers[0].pixels);
			else
				SFG_FREE(buffers[0].pixels);

			for (u8 i = 1; i < levels; ++i)
				SFG_FREE(buffers[i].pixels);
		}

		void write_png_data(void* context, void* data, int size)
		{
			static_cast<ostream_t*>(context)->write_raw(static_cast<const u8*>(data), static_cast<size_t>(size));
		}

		u8 get_texture_cook_level_count(const texture_cook_config_t& cfg, const vec2u16_t& size)
		{
			if (!cfg.generate_mipmaps)
				return 1;

			u8 levels = image_util_t::calculate_mip_levels(size.x, size.y);
			if (levels > texture_loader_t::MAX_MIPS)
				levels = texture_loader_t::MAX_MIPS;
			return levels;
		}

		bool cook_from_buffers(const texture_cook_config_t& cfg, texture_buffer_t* buffers, const vec2u16_t& size, u8 channels, u64 source_tick, const char* source_name, resource_header_t& out_header, ostream_t& stream)
		{
			const u8 levels = get_texture_cook_level_count(cfg, size);
			if (levels > 1)
				image_util_t::generate_mips(buffers, levels, image_util_t::mip_gen_filter::def, channels, cfg.is_linear, false);

			const u8		 is_linear_u8	= cfg.is_linear ? 1 : 0;
			const format_e	 raw_format		= cfg.is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
			const format_e	 runtime_format = cfg.payload_type == texture_payload_type_e::ktx2_uastc ? (cfg.is_linear ? format_e::bc7_block_unorm : format_e::bc7_block_srgb) : raw_format;
			texture_header_t texture_header = {};
			texture_header.texture_format	= runtime_format;
			texture_header.payload_type		= cfg.payload_type;
			texture_header.ktx2_compression = cfg.ktx2_compression;
			texture_header.size				= size;
			texture_header.bpp				= cfg.payload_type == texture_payload_type_e::ktx2_uastc ? 16 : channels;
			texture_header.mip_count		= levels;
			texture_header.is_linear		= is_linear_u8;
			texture_header.use_streaming	= cfg.use_streaming ? 1 : 0;
			texture_header.average_color	= sample_average_color(buffers[0]);

			out_header = {
				.type		 = resource_type_e::texture,
				.magic		 = texture_loader_t::WIRE_MAGIC,
				.version	 = texture_loader_t::WIRE_VERSION,
				.source_tick = source_tick,
			};

			if (cfg.payload_type == texture_payload_type_e::uncompressed)
			{
				ostream_t raw_stream;
				u32		  byte_offset = 0;

				for (u8 i = 0; i < levels; i++)
				{
					const texture_buffer_t& buf = buffers[i];
					texture_header.mips[i]		= {
						.byte_offset = byte_offset,
						.data_size	 = buf.data_size,
						.row_pitch	 = buf.row_pitch,
						.size		 = buf.size,
						.bpp		 = buf.bpp,
					};
					byte_offset += buf.data_size;
					raw_stream.write_raw(buf.pixels, buf.data_size);
				}

				ostream_t compressed = compressor_t::compress(raw_stream);
				if (compressed.get_size() == 0)
				{
					SFG_ERR("failed to compress texture payload for {0}", source_name);
					return false;
				}

				SFG_ASSERT(compressed.get_size() <= UINT32_MAX);
				const u32 blob_size = static_cast<u32>(compressed.get_size());
				stream << texture_header;
				stream << blob_size;
				stream.write_raw(compressed.get_raw(), blob_size);
			}
			else if (cfg.payload_type == texture_payload_type_e::png)
			{
				ostream_t png_streams[texture_loader_t::MAX_MIPS] = {};
				u32		  byte_offset							  = 0;

				for (u8 i = 0; i < levels; i++)
				{
					const texture_buffer_t& buf		   = buffers[i];
					ostream_t&				png_stream = png_streams[i];
					if (stbi_write_png_to_func(write_png_data, &png_stream, buf.size.x, buf.size.y, channels, buf.pixels, buf.row_pitch) == 0 || png_stream.get_size() == 0)
					{
						SFG_ERR("failed to encode PNG texture for {0}", source_name);
						return false;
					}

					SFG_ASSERT(png_stream.get_size() <= UINT32_MAX);
					const u32 blob_size	   = static_cast<u32>(png_stream.get_size());
					texture_header.mips[i] = {
						.byte_offset = byte_offset,
						.data_size	 = blob_size,
						.row_pitch	 = buf.row_pitch,
						.size		 = buf.size,
						.bpp		 = buf.bpp,
					};
					byte_offset += blob_size;
				}

				stream << texture_header;
				for (u8 i = 0; i < levels; i++)
				{
					const ostream_t& png_stream = png_streams[i];
					stream.write_raw(png_stream.get_raw(), png_stream.get_size());
				}
			}
			else
			{
				ktxTextureCreateInfo create_info = {};
				create_info.vkFormat			 = cfg.is_linear ? SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM : SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB;
				create_info.baseWidth			 = size.x;
				create_info.baseHeight			 = size.y;
				create_info.baseDepth			 = 1;
				create_info.numDimensions		 = 2;
				create_info.numLevels			 = levels;
				create_info.numLayers			 = 1;
				create_info.numFaces			 = 1;

				ktxTexture2*   ktx_texture = nullptr;
				KTX_error_code ktx_result  = ktxTexture2_Create(&create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx_texture);
				if (ktx_result == KTX_SUCCESS)
				{
					for (u8 i = 0; i < levels; i++)
					{
						const texture_buffer_t& buf = buffers[i];
						ktx_result					= ktxTexture_SetImageFromMemory(ktxTexture(ktx_texture), i, 0, 0, buf.pixels, buf.data_size);
						if (ktx_result != KTX_SUCCESS)
							break;
					}
				}

				if (ktx_result == KTX_SUCCESS)
				{
					ktx_pack_uastc_flags uastc_flags = KTX_PACK_UASTC_LEVEL_DEFAULT;
					switch (cfg.ktx2_compression)
					{
					case texture_ktx2_compression_e::fastest:
						uastc_flags = KTX_PACK_UASTC_LEVEL_FASTEST;
						break;
					case texture_ktx2_compression_e::faster:
						uastc_flags = KTX_PACK_UASTC_LEVEL_FASTER;
						break;
					case texture_ktx2_compression_e::default_quality:
						uastc_flags = KTX_PACK_UASTC_LEVEL_DEFAULT;
						break;
					case texture_ktx2_compression_e::high_quality:
						uastc_flags = KTX_PACK_UASTC_LEVEL_SLOWER;
						break;
					}

					ktxBasisParams basis_params = {};
					basis_params.structSize		= sizeof(basis_params);
					basis_params.uastc			= KTX_TRUE;
					basis_params.uastcFlags		= uastc_flags | KTX_PACK_UASTC_FAVOR_BC7_ERROR;
					basis_params.threadCount	= 4;
					ktx_result					= ktxTexture2_CompressBasisEx(ktx_texture, &basis_params);
				}

				if (ktx_result == KTX_SUCCESS && cfg.ktx2_compression != texture_ktx2_compression_e::fastest)
				{
					u32 zstd_level = SFG_KTX_ZSTD_COMPRESSION_DEFAULT;
					switch (cfg.ktx2_compression)
					{
					case texture_ktx2_compression_e::fastest:
						break;
					case texture_ktx2_compression_e::faster:
						zstd_level = SFG_KTX_ZSTD_COMPRESSION_FASTER;
						break;
					case texture_ktx2_compression_e::default_quality:
						zstd_level = SFG_KTX_ZSTD_COMPRESSION_DEFAULT;
						break;
					case texture_ktx2_compression_e::high_quality:
						zstd_level = SFG_KTX_ZSTD_COMPRESSION_HIGH;
						break;
					}

					ktx_result = ktxTexture2_DeflateZstd(ktx_texture, zstd_level);
				}

				ktx_uint8_t* ktx_bytes = nullptr;
				ktx_size_t	 ktx_size  = 0;
				if (ktx_result == KTX_SUCCESS)
					ktx_result = ktxTexture2_WriteToMemory(ktx_texture, &ktx_bytes, &ktx_size);

				if (ktx_result != KTX_SUCCESS)
				{
					SFG_ERR("failed to encode KTX2 UASTC texture for {0}: {1}", source_name, ktxErrorString(ktx_result));
					if (ktx_texture != nullptr)
						ktxTexture2_Destroy(ktx_texture);
					return false;
				}

				SFG_ASSERT(ktx_size <= UINT32_MAX);
				const u32 blob_size	   = static_cast<u32>(ktx_size);
				texture_header.mips[0] = {
					.byte_offset = 0,
					.data_size	 = blob_size,
					.row_pitch	 = buffers[0].row_pitch,
					.size		 = buffers[0].size,
					.bpp		 = buffers[0].bpp,
				};
				stream << texture_header;
				stream.write_raw(ktx_bytes, blob_size);

				SFG_FREE(ktx_bytes);
				ktxTexture2_Destroy(ktx_texture);
			}

			return true;
		}
	}

	bool texture_cooker::cook_from_file(const texture_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		const u64 source_tick = file_system_t::get_last_modified_ticks(full_path);
		vec2u16_t size		  = {};
		u8		  channels	  = 4;
		void*	  raw_image	  = cfg.payload_type != texture_payload_type_e::ktx2_uastc && !cfg.force_4_channels ? image_util_t::load_from_file(full_path, size, channels) : image_util_t::load_from_file_ch(full_path, size, 4);
		if (raw_image == nullptr)
		{
			SFG_ERR("failed to load texture source: {0}", full_path);
			return false;
		}

		texture_buffer_t buffers[texture_loader_t::MAX_MIPS] = {};
		buffers[0].pixels									 = static_cast<u8*>(raw_image);
		buffers[0].size										 = size;
		buffers[0].bpp										 = channels;
		buffers[0].row_pitch								 = static_cast<u32>(size.x) * channels;
		buffers[0].data_size								 = buffers[0].row_pitch * size.y;

		const bool result = cook_from_buffers(cfg, buffers, size, channels, source_tick, full_path, out_header, stream);
		free_texture_buffers(buffers, get_texture_cook_level_count(cfg, size), true);
		return result;
	}

	bool texture_cooker::cook_from_data(const texture_cook_config_t& cfg, span_t<u8> data, resource_header_t& out_header, ostream_t& stream)
	{
		SFG_ASSERT(cfg.size.x != 0);
		SFG_ASSERT(cfg.size.y != 0);

		const size_t expected_size = static_cast<size_t>(cfg.size.x) * static_cast<size_t>(cfg.size.y) * 4;
		SFG_ASSERT(data.size == expected_size);
		SFG_ASSERT(expected_size <= UINT32_MAX);

		u8* pixels = static_cast<u8*>(SFG_MALLOC(data.size));
		SFG_MEMCPY(pixels, data.data, data.size);

		texture_buffer_t buffers[texture_loader_t::MAX_MIPS] = {};
		buffers[0].pixels									 = pixels;
		buffers[0].size										 = cfg.size;
		buffers[0].bpp										 = 4;
		buffers[0].row_pitch								 = static_cast<u32>(cfg.size.x) * 4;
		buffers[0].data_size								 = static_cast<u32>(expected_size);

		const bool result = cook_from_buffers(cfg, buffers, cfg.size, 4, hashing_t::hash_u64(data.data, data.size), "raw texture data", out_header, stream);
		free_texture_buffers(buffers, get_texture_cook_level_count(cfg, cfg.size), false);
		return result;
	}

#undef SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM
#undef SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB
#undef SFG_KTX_ZSTD_COMPRESSION_FASTER
#undef SFG_KTX_ZSTD_COMPRESSION_DEFAULT
#undef SFG_KTX_ZSTD_COMPRESSION_HIGH
}

namespace sfg
{
	texture_cook_config_reflection_t::texture_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "texture_cook_config_t",
			.display_name = "Texture Cook Config",
			.fields =
				{
					{.name = "size", .display_name = "Size", .sub_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(texture_cook_config_t, size), .size = sizeof(vec2u16_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::object},
					{.name		   = "payload_type",
					 .display_name = "Payload Type",
					 .sub_type_id  = type_id_t<texture_payload_type_e>::value,
					 .offset	   = offsetof(texture_cook_config_t, payload_type),
					 .size		   = sizeof(texture_payload_type_e),
					 .type		   = reflected_value_type_e::u8},
					{.name		   = "ktx2_compression",
					 .display_name = "KTX2 Compression",
					 .sub_type_id  = type_id_t<texture_ktx2_compression_e>::value,
					 .offset	   = offsetof(texture_cook_config_t, ktx2_compression),
					 .size		   = sizeof(texture_ktx2_compression_e),
					 .type		   = reflected_value_type_e::u8},
					{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .offset = offsetof(texture_cook_config_t, generate_mipmaps), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "is_linear", .display_name = "Linear", .offset = offsetof(texture_cook_config_t, is_linear), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "use_streaming", .display_name = "Use Streaming", .offset = offsetof(texture_cook_config_t, use_streaming), .size = sizeof(bool), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::boolean},
					{.name = "force_4_channels", .display_name = "Force 4 Channels", .offset = offsetof(texture_cook_config_t, force_4_channels), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<texture_cook_config_t>::value,
			.size	   = sizeof(texture_cook_config_t),
			.alignment = alignof(texture_cook_config_t),
		});
	}
}
