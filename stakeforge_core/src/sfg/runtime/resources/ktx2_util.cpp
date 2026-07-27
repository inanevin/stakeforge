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

#include "ktx2_util.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>

#include <ktx.h>

namespace sfg
{
#define SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM  37
#define SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB	  43
#define SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK 145
#define SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK  146
#define SFG_KTX_ZSTD_COMPRESSION_FASTER	  1
#define SFG_KTX_ZSTD_COMPRESSION_DEFAULT  3
#define SFG_KTX_ZSTD_COMPRESSION_HIGH	  5

	bool ktx2_util_t::encode_uastc(span_t<const texture_buffer_t> mips, bool is_linear, texture_ktx2_compression_e compression, const char* source_name, ostream_t& stream)
	{
		SFG_ASSERT(mips.size != 0);
		SFG_ASSERT(mips.size <= UINT8_MAX);

		const texture_buffer_t&	   base_mip	   = mips.data[0];
		const ktxTextureCreateInfo create_info = {
			.vkFormat	   = static_cast<ktx_uint32_t>(is_linear ? SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM : SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB),
			.baseWidth	   = base_mip.size.x,
			.baseHeight	   = base_mip.size.y,
			.baseDepth	   = 1,
			.numDimensions = 2,
			.numLevels	   = static_cast<ktx_uint32_t>(mips.size),
			.numLayers	   = 1,
			.numFaces	   = 1,
		};

		ktxTexture2*   ktx_texture = nullptr;
		KTX_error_code ktx_result  = ktxTexture2_Create(&create_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx_texture);

		if (ktx_result == KTX_SUCCESS)
		{
			for (u8 i = 0; i < mips.size; ++i)
			{
				const texture_buffer_t& mip = mips.data[i];
				ktx_result					= ktxTexture_SetImageFromMemory(ktxTexture(ktx_texture), i, 0, 0, mip.pixels, mip.data_size);

				if (ktx_result != KTX_SUCCESS)
					break;
			}
		}

		if (ktx_result == KTX_SUCCESS)
		{
			ktx_pack_uastc_flags uastc_flags = KTX_PACK_UASTC_LEVEL_DEFAULT;

			switch (compression)
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

		if (ktx_result == KTX_SUCCESS && compression != texture_ktx2_compression_e::fastest)
		{
			u32 zstd_level = SFG_KTX_ZSTD_COMPRESSION_DEFAULT;

			switch (compression)
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
			SFG_ERR("failed to encode KTX2 UASTC image for {0}: {1}", source_name, ktxErrorString(ktx_result));

			if (ktx_texture != nullptr)
				ktxTexture2_Destroy(ktx_texture);

			return false;
		}

		stream.write_raw(ktx_bytes, ktx_size);

		SFG_FREE(ktx_bytes);
		ktxTexture2_Destroy(ktx_texture);

		return true;
	}

	bool ktx2_util_t::decode_uastc(span_t<const u8> data, texture_ktx2_compression_e compression, u64 resource_hash, texture_buffer_t* out_mips, u8 max_mips, ktx2_image_desc_t& out_desc)
	{
		ktxTexture2*   ktx_texture = nullptr;
		KTX_error_code ktx_result  = ktxTexture2_CreateFromMemory(data.data, data.size, 0, &ktx_texture);

		if (ktx_result == KTX_SUCCESS && compression == texture_ktx2_compression_e::fastest)
			SFG_ASSERT(ktx_texture->supercompressionScheme != KTX_SS_ZSTD);

		if (ktx_result == KTX_SUCCESS)
			ktx_result = ktxTexture2_LoadImageData(ktx_texture, nullptr, 0);

		if (ktx_result == KTX_SUCCESS && ktxTexture2_NeedsTranscoding(ktx_texture))
			ktx_result = ktxTexture2_TranscodeBasis(ktx_texture, KTX_TTF_BC7_RGBA, KTX_TF_HIGH_QUALITY);

		if (ktx_result != KTX_SUCCESS)
		{
			SFG_ERR("failed to transcode KTX2 image {0}: {1}", resource_hash, ktxErrorString(ktx_result));

			if (ktx_texture != nullptr)
				ktxTexture2_Destroy(ktx_texture);

			return false;
		}

		switch (ktx_texture->vkFormat)
		{
		case SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK:
			out_desc.format = format_e::bc7_block_srgb;
			break;
		case SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK:
			out_desc.format = format_e::bc7_block_unorm;
			break;
		case SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB:
			out_desc.format = format_e::r8g8b8a8_srgb;
			break;
		case SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM:
			out_desc.format = format_e::r8g8b8a8_unorm;
			break;
		default:
			out_desc.format = format_e::undefined;
			break;
		}

		if (out_desc.format == format_e::undefined)
		{
			SFG_ERR("unsupported KTX2 transcode format for image {0}", resource_hash);
			ktxTexture2_Destroy(ktx_texture);
			return false;
		}

		out_desc.mip_count = static_cast<u8>(ktx_texture->numLevels);

		if (out_desc.mip_count > max_mips)
		{
			SFG_ERR("KTX2 image {0} has too many mip levels", resource_hash);
			ktxTexture2_Destroy(ktx_texture);
			return false;
		}

		out_desc.size = vec2u16_t(static_cast<u16>(ktx_texture->baseWidth), static_cast<u16>(ktx_texture->baseHeight));
		const u8 bpp  = format_is_block_compressed(out_desc.format) ? 16 : format_get_bpp(out_desc.format);

		for (u8 i = 0; i < out_desc.mip_count; ++i)
		{
			ktx_size_t offset = 0;
			ktx_result		  = ktxTexture_GetImageOffset(ktxTexture(ktx_texture), i, 0, 0, &offset);

			if (ktx_result != KTX_SUCCESS)
			{
				SFG_ERR("failed to get KTX2 image offset for {0}: {1}", resource_hash, ktxErrorString(ktx_result));
				release(out_mips, i);
				ktxTexture2_Destroy(ktx_texture);
				return false;
			}

			const ktx_size_t image_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), i);

			if (image_size > UINT32_MAX)
			{
				SFG_ERR("KTX2 image {0} is too large", resource_hash);
				release(out_mips, i);
				ktxTexture2_Destroy(ktx_texture);
				return false;
			}

			texture_buffer_t& mip	 = out_mips[i];
			const u32		  width	 = ktx_texture->baseWidth >> i;
			const u32		  height = ktx_texture->baseHeight >> i;
			mip.data_size			 = static_cast<u32>(image_size);
			mip.row_pitch			 = ktxTexture_GetRowPitch(ktxTexture(ktx_texture), i);
			mip.size				 = vec2u16_t(static_cast<u16>(width == 0 ? 1 : width), static_cast<u16>(height == 0 ? 1 : height));
			mip.bpp					 = bpp;
			mip.pixels				 = static_cast<u8*>(SFG_MALLOC(mip.data_size));

			if (mip.pixels == nullptr)
			{
				SFG_ERR("failed to allocate KTX2 image pixels for {0}", resource_hash);
				release(out_mips, i);
				ktxTexture2_Destroy(ktx_texture);
				return false;
			}

			SFG_MEMCPY(mip.pixels, ktxTexture_GetData(ktxTexture(ktx_texture)) + offset, mip.data_size);
		}

		ktxTexture2_Destroy(ktx_texture);

		return true;
	}

	void ktx2_util_t::release(texture_buffer_t* mips, u8 mip_count)
	{
		for (u8 i = 0; i < mip_count; ++i)
		{
			SFG_FREE(mips[i].pixels);
			mips[i].pixels = nullptr;
		}
	}

#undef SFG_KTX_VK_FORMAT_R8G8B8A8_UNORM
#undef SFG_KTX_VK_FORMAT_R8G8B8A8_SRGB
#undef SFG_KTX_VK_FORMAT_BC7_UNORM_BLOCK
#undef SFG_KTX_VK_FORMAT_BC7_SRGB_BLOCK
#undef SFG_KTX_ZSTD_COMPRESSION_FASTER
#undef SFG_KTX_ZSTD_COMPRESSION_DEFAULT
#undef SFG_KTX_ZSTD_COMPRESSION_HIGH
}
