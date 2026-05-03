// Copyright (c) 2025 Inan Evin

#include "texture_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	bool texture_cook_from_file(const char* full_path, const texture_config_t& cfg, texture_cook_t& out)
	{
		vec2u16_t size		= {};
		void*	  raw_image = image_util_t::load_from_file_ch(full_path, size, 4);
		if (raw_image == nullptr)
			return false;

		out			  = texture_cook_t{};
		out.width	  = size.x;
		out.height	  = size.y;
		out.channels  = 4;
		out.is_linear = cfg.is_linear;

		texture_buffer_t buffers[texture_max_mips] = {};
		buffers[0].pixels						   = static_cast<u8*>(raw_image);
		buffers[0].size							   = size;
		buffers[0].bpp							   = 4;

		u8 levels = 1;
		if (cfg.generate_mipmaps)
		{
			levels = image_util_t::calculate_mip_levels(size.x, size.y);
			if (levels > texture_max_mips)
				levels = texture_max_mips;
			if (levels > 1)
				image_util_t::generate_mips(buffers, levels, image_util_t::mip_gen_filter::def, 4, cfg.is_linear, false);
		}

		out.mip_count = levels;

		for (u8 i = 0; i < levels; ++i)
		{
			texture_cook_mip_t&		dst = out.mips[i];
			const texture_buffer_t& src = buffers[i];
			dst.width					= src.size.x;
			dst.height					= src.size.y;
			const u32 byte_count		= static_cast<u32>(src.size.x) * static_cast<u32>(src.size.y) * static_cast<u32>(src.bpp);
			dst.pixels.resize(byte_count);
			SFG_MEMCPY(dst.pixels.data(), src.pixels, byte_count);
		}

		image_util_t::free(buffers[0].pixels);
		for (u8 i = 1; i < levels; ++i)
			SFG_FREE(buffers[i].pixels);

		return true;
	}

	bool texture_cook_serialize(const texture_cook_t& src, ostream_t& stream)
	{
		u32 total					  = 0;
		u32 offsets[texture_max_mips] = {0};
		for (u8 i = 0; i < src.mip_count; ++i)
		{
			offsets[i] = total;
			total += static_cast<u32>(src.mips[i].pixels.size());
		}

		const u8 is_linear_u8 = src.is_linear ? 1 : 0;

		stream << texture_wire_magic;
		stream << texture_wire_version;
		stream << total;
		stream << src.width << src.height;
		stream << src.channels << is_linear_u8 << src.mip_count;

		for (u8 i = 0; i < src.mip_count; ++i)
		{
			const u32 size = static_cast<u32>(src.mips[i].pixels.size());
			stream << offsets[i] << size;
			stream << src.mips[i].width << src.mips[i].height;
		}

		for (u8 i = 0; i < src.mip_count; ++i)
		{
			const texture_cook_mip_t& m = src.mips[i];
			if (!m.pixels.empty())
				stream.write_raw(m.pixels.data(), m.pixels.size());
		}

		return true;
	}
}
