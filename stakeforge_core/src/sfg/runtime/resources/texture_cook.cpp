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
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "texture.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/memory.hpp>
#include <cstdint>
#include <cstdlib>
#include <ktx.h>

namespace sfg
{
#define SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM 37
#define SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB	 43
#define SFG_KTX_ZSTD_COMPRESSION_LEVEL	 3

	namespace
	{
		void free_texture_buffers(texture_buffer_t* buffers, u8 levels, bool base_from_image_util)
		{
			if (base_from_image_util)
				image_util_t::free(buffers[0].pixels);
			else
				SFG_FREE(buffers[0].pixels);

			for (u8 i = 1; i < levels; ++i)
				SFG_FREE(buffers[i].pixels);
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

		bool cook_from_buffers(const texture_cook_config_t& cfg, texture_buffer_t* buffers, const vec2u16_t& size, u64 source_tick, const char* source_name, resource_header_t& out_header, ostream_t& stream)
		{
			const u8 levels = get_texture_cook_level_count(cfg, size);
			if (levels > 1)
				image_util_t::generate_mips(buffers, levels, image_util_t::mip_gen_filter::def, 4, cfg.is_linear, false);

			const u8	   is_linear_u8 = cfg.is_linear ? 1 : 0;
			const u8	   channels		= 4;
			const format_e raw_format	= cfg.is_linear ? format_e::r8g8b8a8_unorm : format_e::r8g8b8a8_srgb;
			out_header					= {
								 .magic		  = texture_loader_t::WIRE_MAGIC,
								 .version	  = texture_loader_t::WIRE_VERSION,
								 .source_tick = source_tick,
			 };

			stream << cfg.payload_type << channels << is_linear_u8;

			if (cfg.payload_type == texture_payload_type_e::uncompressed)
			{
				stream << raw_format << levels;

				for (u8 i = 0; i < levels; i++)
				{
					const texture_buffer_t& buf = buffers[i];
					stream << buf.bpp;
					stream << buf.size;
					stream << buf.row_pitch;
					stream << buf.data_size;
					if (buf.pixels != nullptr && buf.data_size != 0)
						stream.write_raw(buf.pixels, buf.data_size);
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
					ktxBasisParams basis_params = {};
					basis_params.structSize		= sizeof(basis_params);
					basis_params.uastc			= KTX_TRUE;
					basis_params.uastcFlags		= KTX_PACK_UASTC_LEVEL_DEFAULT | KTX_PACK_UASTC_FAVOR_BC7_ERROR;
					basis_params.threadCount	= 4;
					ktx_result					= ktxTexture2_CompressBasisEx(ktx_texture, &basis_params);
				}

				if (ktx_result == KTX_SUCCESS)
					ktx_result = ktxTexture2_DeflateZstd(ktx_texture, SFG_KTX_ZSTD_COMPRESSION_LEVEL);

				ktx_uint8_t* ktx_bytes = nullptr;
				ktx_size_t	 ktx_size  = 0;
				if (ktx_result == KTX_SUCCESS)
					ktx_result = ktxTexture2_WriteToMemory(ktx_texture, &ktx_bytes, &ktx_size);

				if (ktx_result != KTX_SUCCESS)
				{
					SFG_ERR("KTX2 UASTC encoding failed for {0}: {1}", source_name, ktxErrorString(ktx_result));
					if (ktx_texture != nullptr)
						ktxTexture2_Destroy(ktx_texture);
					return false;
				}

				SFG_ASSERT(ktx_size <= UINT32_MAX);
				const u32 blob_size = static_cast<u32>(ktx_size);
				stream << blob_size;
				stream.write_raw(ktx_bytes, blob_size);

				std::free(ktx_bytes);
				ktxTexture2_Destroy(ktx_texture);
			}

			return true;
		}
	}

	bool texture_cooker::cook_from_file(const texture_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		vec2u16_t size		= {};
		void*	  raw_image = image_util_t::load_from_file_ch(full_path, size, 4);
		if (raw_image == nullptr)
			return false;

		texture_buffer_t buffers[texture_loader_t::MAX_MIPS] = {};
		buffers[0].pixels									 = static_cast<u8*>(raw_image);
		buffers[0].size										 = size;
		buffers[0].bpp										 = 4;
		buffers[0].row_pitch								 = static_cast<u32>(size.x) * 4;
		buffers[0].data_size								 = buffers[0].row_pitch * size.y;

		const bool result = cook_from_buffers(cfg, buffers, size, file_system_t::get_last_modified_ticks(full_path), full_path, out_header, stream);
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

		const bool result = cook_from_buffers(cfg, buffers, cfg.size, hashing_t::hash_u64(data.data, data.size), "raw texture data", out_header, stream);
		free_texture_buffers(buffers, get_texture_cook_level_count(cfg, cfg.size), false);
		return result;
	}

#undef SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM
#undef SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB
#undef SFG_KTX_ZSTD_COMPRESSION_LEVEL
}

namespace sfg
{
	texture_cook_config_reflection_t::texture_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<texture_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "size", .display_name = "Size", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(texture_cook_config_t, size), .size = sizeof(vec2u16_t), .flags = reflected_field_flags_no_ui},
			{.name		   = "payload_type",
			 .display_name = "Payload Type",
			 .type		   = reflected_value_type_e::enum8,
			 .sub_type_id  = type_id_t<texture_payload_type_e>::value,
			 .offset	   = offsetof(texture_cook_config_t, payload_type),
			 .size		   = sizeof(texture_payload_type_e)},
			{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, generate_mipmaps), .size = sizeof(bool)},
			{.name = "is_linear", .display_name = "Linear", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, is_linear), .size = sizeof(bool)},
		};

		registry.register_type({
			.fields		  = {.data = fields, .size = std::size(fields)},
			.name		  = "texture_cook_config_t",
			.display_name = "Texture Cook Config",
			.type_id	  = type_id_t<texture_cook_config_t>::value,
			.size		  = sizeof(texture_cook_config_t),
			.alignment	  = alignof(texture_cook_config_t),
		});
	}
}
