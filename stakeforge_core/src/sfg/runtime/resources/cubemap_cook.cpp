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

#include "cubemap_cook.hpp"
#include "cubemap_data.hpp"

#include <sfg/common/packing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/stb/stb_image.h>

namespace sfg
{
	namespace
	{
		struct hdr_source_t
		{
			float* data	  = nullptr;
			i32	   width  = 0;
			i32	   height = 0;
		};

		vec3f_t get_face_direction(u8 face, u16 x, u16 y, vec2u16_t size)
		{
			const f32 u = (2.0f * (static_cast<f32>(x) + 0.5f) / static_cast<f32>(size.x)) - 1.0f;
			const f32 v = (2.0f * (static_cast<f32>(y) + 0.5f) / static_cast<f32>(size.y)) - 1.0f;

			switch (face)
			{
			case 0:
				return vec3f_t(1.0f, -v, -u).normalized();
			case 1:
				return vec3f_t(-1.0f, -v, u).normalized();
			case 2:
				return vec3f_t(u, 1.0f, v).normalized();
			case 3:
				return vec3f_t(u, -1.0f, -v).normalized();
			case 4:
				return vec3f_t(u, -v, 1.0f).normalized();
			default:
				return vec3f_t(-u, -v, -1.0f).normalized();
			}
		}

		vec3f_t get_source_pixel(const hdr_source_t& source, i32 x, i32 y)
		{
			if (x < 0)
				x = (x % source.width) + source.width;

			if (x >= source.width)
				x = x % source.width;

			y				   = static_cast<i32>(math::clamp(static_cast<f32>(y), 0.0f, static_cast<f32>(source.height - 1)));
			const size_t pixel = (static_cast<size_t>(y) * static_cast<size_t>(source.width) + static_cast<size_t>(x)) * 3;
			return {source.data[pixel], source.data[pixel + 1], source.data[pixel + 2]};
		}

		vec3f_t sample_equirect(const hdr_source_t& source, const vec3f_t& direction)
		{
			const f32	  u	  = static_cast<f32>(std::atan2(direction.z, direction.x)) * MATH_R_TWO_PI + 0.5f;
			const f32	  v	  = math::acos(math::clamp(direction.y, -1.0f, 1.0f)) * MATH_R_PI;
			const f32	  x	  = u * static_cast<f32>(source.width) - 0.5f;
			const f32	  y	  = v * static_cast<f32>(source.height) - 0.5f;
			const i32	  x0  = static_cast<i32>(math::floor(x));
			const i32	  y0  = static_cast<i32>(math::floor(y));
			const i32	  x1  = x0 + 1;
			const i32	  y1  = y0 + 1;
			const f32	  tx  = x - static_cast<f32>(x0);
			const f32	  ty  = y - static_cast<f32>(y0);
			const vec3f_t c00 = get_source_pixel(source, x0, y0);
			const vec3f_t c10 = get_source_pixel(source, x1, y0);
			const vec3f_t c01 = get_source_pixel(source, x0, y1);
			const vec3f_t c11 = get_source_pixel(source, x1, y1);
			const vec3f_t cx0 = c00 * (1.0f - tx) + c10 * tx;
			const vec3f_t cx1 = c01 * (1.0f - tx) + c11 * tx;
			return cx0 * (1.0f - ty) + cx1 * ty;
		}

		bool make_buffer(vec2u16_t size, texture_buffer_t& buffer)
		{
			buffer				   = {};
			buffer.size			   = size;
			buffer.bpp			   = format_get_bpp(format_e::r16g16b16a16_sfloat);
			const size_t row_pitch = static_cast<size_t>(size.x) * buffer.bpp;
			const size_t data_size = row_pitch * size.y;

			if (row_pitch > UINT32_MAX || data_size > UINT32_MAX)
				return false;

			buffer.row_pitch = static_cast<u32>(row_pitch);
			buffer.data_size = static_cast<u32>(data_size);
			buffer.pixels	 = static_cast<u8*>(SFG_MALLOC(buffer.data_size));
			return buffer.pixels != nullptr;
		}

		void write_pixel(texture_buffer_t& buffer, u16 x, u16 y, const vec3f_t& color)
		{
			u16* pixel = reinterpret_cast<u16*>(buffer.pixels + static_cast<size_t>(y) * buffer.row_pitch + static_cast<size_t>(x) * buffer.bpp);
			pixel[0]   = packing_t::float_to_half(math::max(color.x, 0.0f));
			pixel[1]   = packing_t::float_to_half(math::max(color.y, 0.0f));
			pixel[2]   = packing_t::float_to_half(math::max(color.z, 0.0f));
			pixel[3]   = packing_t::float_to_half(1.0f);
		}

		bool build_cubemap(const hdr_source_t& source, vec2u16_t size, cubemap_texture_block_t& block)
		{
			block			= {};
			block.format	= format_e::r16g16b16a16_sfloat;
			block.size		= size;
			block.mip_count = 1;

			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			{
				texture_buffer_t& buffer = block.buffers[face * cubemap_loader_t::MAX_MIPS];

				if (!make_buffer(size, buffer))
				{
					for (u8 allocated_face = 0; allocated_face < face; ++allocated_face)
						SFG_FREE(block.buffers[allocated_face * cubemap_loader_t::MAX_MIPS].pixels);

					return false;
				}

				for (u16 y = 0; y < size.y; ++y)
				{
					for (u16 x = 0; x < size.x; ++x)
						write_pixel(buffer, x, y, sample_equirect(source, get_face_direction(face, x, y, size)));
				}
			}

			return true;
		}

		void write_texture_block(const cubemap_texture_block_t& block, ostream_t& stream)
		{
			stream << block.format;
			stream << block.size;
			stream << block.mip_count;

			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					const texture_buffer_t& buffer = block.buffers[face * cubemap_loader_t::MAX_MIPS + mip];
					stream << buffer.size;
					stream << buffer.row_pitch;
					stream << buffer.data_size;
					stream.write_raw(buffer.pixels, buffer.data_size);
				}
			}
		}

		void free_texture_block(cubemap_texture_block_t& block)
		{
			for (u8 face = 0; face < cubemap_loader_t::FACE_COUNT; ++face)
			{
				for (u8 mip = 0; mip < block.mip_count; ++mip)
				{
					texture_buffer_t& buffer = block.buffers[face * cubemap_loader_t::MAX_MIPS + mip];
					SFG_FREE(buffer.pixels);
					buffer.pixels = nullptr;
				}
			}
		}
	}

	bool cubemap_cooker::cook_from_file(const cubemap_cook_config_t& config, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		if (config.size.x == 0 || config.size.y == 0)
		{
			SFG_ERR("invalid cubemap cook config");
			return false;
		}

		int	   width	= 0;
		int	   height	= 0;
		int	   channels = 0;
		float* data		= stbi_loadf(full_path, &width, &height, &channels, 3);

		if (data == nullptr || width <= 0 || height <= 0)
		{
			if (data != nullptr)
				stbi_image_free(data);

			SFG_ERR("failed to load cubemap source {0}", full_path);
			return false;
		}

		const hdr_source_t		source = {.data = data, .width = width, .height = height};
		cubemap_texture_block_t block  = {};

		if (!build_cubemap(source, config.size, block))
		{
			stbi_image_free(data);
			SFG_ERR("failed to allocate cubemap faces for {0}", full_path);
			return false;
		}

		stbi_image_free(data);

		out_header = {
			.type		 = resource_type_e::cubemap,
			.magic		 = cubemap_loader_t::WIRE_MAGIC,
			.version	 = cubemap_loader_t::WIRE_VERSION,
			.source_tick = file_system_t::get_last_modified_ticks(full_path),
		};

		ostream_t payload = {};
		write_texture_block(block, payload);

		stream = compressor_t::compress(payload);

		if (stream.get_size() == 0)
			SFG_ERR("failed to compress cubemap payload: {0}", full_path);

		free_texture_block(block);
		return stream.get_size() != 0;
	}

	cubemap_cook_config_reflection_t::cubemap_cook_config_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name		  = "cubemap_cook_config_t",
			.display_name = "Cubemap Cook Config",
			.fields =
				{
					{.name = "size", .display_name = "Face Size", .sub_type_id = type_id_t<vec2u16_t>::value, .offset = offsetof(cubemap_cook_config_t, size), .size = sizeof(vec2u16_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<cubemap_cook_config_t>::value,
			.size	   = sizeof(cubemap_cook_config_t),
			.alignment = alignof(cubemap_cook_config_t),
		});
	}
}
