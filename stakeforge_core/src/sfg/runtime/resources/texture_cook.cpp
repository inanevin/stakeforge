// Copyright (c) 2025 Inan Evin

#include "texture_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool texture_cooker::cook_from_file(const texture_cook_config_t& cfg, const char* full_path, ostream_t& stream)
	{
		vec2u16_t size		= {};
		void*	  raw_image = image_util_t::load_from_file_ch(full_path, size, 4);
		if (raw_image == nullptr)
			return false;

		texture_buffer_t buffers[texture_loader_t::MAX_MIPS] = {};
		buffers[0].pixels									 = static_cast<u8*>(raw_image);
		buffers[0].size										 = size;
		buffers[0].bpp										 = 4;

		u8 levels = 1;
		if (cfg.generate_mipmaps)
		{
			levels = image_util_t::calculate_mip_levels(size.x, size.y);
			if (levels > texture_loader_t::MAX_MIPS)
				levels = texture_loader_t::MAX_MIPS;
			if (levels > 1)
				image_util_t::generate_mips(buffers, levels, image_util_t::mip_gen_filter::def, 4, cfg.is_linear, false);
		}

		u32 offsets[texture_loader_t::MAX_MIPS] = {0};
		u32 sizes[texture_loader_t::MAX_MIPS]	= {0};
		u32 total								= 0;
		for (u8 i = 0; i < levels; ++i)
		{
			offsets[i] = total;
			sizes[i]   = static_cast<u32>(buffers[i].size.x) * static_cast<u32>(buffers[i].size.y) * static_cast<u32>(buffers[i].bpp);
			total += sizes[i];
		}

		const u8 is_linear_u8 = cfg.is_linear ? 1 : 0;
		const u8 channels	  = 4;

		const size_t	  header_pos = stream.get_size();
		resource_header_t header	 = {
				.magic			= texture_loader_t::WIRE_MAGIC,
				.version		= texture_loader_t::WIRE_VERSION,
				.payload_size	= total,
				.modified_ticks = file_system_t::get_last_modified_ticks(full_path),
		};
		header.serialize(stream);

		stream << static_cast<u32>(size.x) << static_cast<u32>(size.y);
		stream << channels << is_linear_u8 << levels;

		for (u8 i = 0; i < levels; ++i)
		{
			stream << offsets[i] << sizes[i];
			stream << static_cast<u32>(buffers[i].size.x) << static_cast<u32>(buffers[i].size.y);
		}

		header.payload_offset = static_cast<u32>(stream.get_size() - header_pos);
		header.patch_payload_offset(stream, header_pos);

		for (u8 i = 0; i < levels; ++i)
		{
			if (sizes[i] != 0)
				stream.write_raw(buffers[i].pixels, sizes[i]);
		}

		image_util_t::free(buffers[0].pixels);
		for (u8 i = 1; i < levels; ++i)
			image_util_t::free(buffers[i].pixels);

		return true;
	}

	void from_json(const nlohmann::json& j, texture_cook_config_t& c)
	{
		c.generate_mipmaps = j.value<bool>("generate_mipmaps", false);
		c.is_linear		   = j.value<bool>("is_linear", false);
	}
}
