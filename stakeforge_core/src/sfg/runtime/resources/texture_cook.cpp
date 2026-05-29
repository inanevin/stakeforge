// Copyright (c) 2025 Inan Evin

#include "texture_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
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

		const u8		  is_linear_u8 = cfg.is_linear ? 1 : 0;
		const u8		  channels	   = 4;
		resource_header_t header	   = {
				  .magic		= texture_loader_t::WIRE_MAGIC,
				  .version		= texture_loader_t::WIRE_VERSION,
				  .source_ticks = {file_system_t::get_last_modified_ticks(full_path)},
		  };
		header.serialize(stream);

		stream << channels << is_linear_u8 << levels;

		for (u8 i = 0; i < levels; i++)
		{
			const texture_buffer_t& buf = buffers[i];
			stream << buf.bpp;
			stream << buf.size;
			const size_t sz = buf.bpp * buf.size.x * buf.size.y;
			if (buf.pixels != nullptr && sz != 0)
				stream.write_raw(buf.pixels, sz);
		}

		image_util_t::free(buffers[0].pixels);
		for (u8 i = 1; i < levels; ++i)
			image_util_t::free(buffers[i].pixels);

		return true;
	}

	void to_json(nlohmann::json& j, const texture_cook_payload_type_e& e)
	{
		switch (e)
		{
		case texture_cook_payload_type_e::uncompressed:
			j = "uncompressed";
			return;
		case texture_cook_payload_type_e::ktx2_uastc:
			j = "ktx2_uastc";
			return;
		}

		j = "uncompressed";
	}

	void from_json(const nlohmann::json& j, texture_cook_payload_type_e& e)
	{
		const string_t str = j.get<string_t>();

		if (str.compare("uncompressed") == 0)
		{
			e = texture_cook_payload_type_e::uncompressed;
			return;
		}
		if (str.compare("ktx2_uastc") == 0)
		{
			e = texture_cook_payload_type_e::ktx2_uastc;
			return;
		}

		e = texture_cook_payload_type_e::uncompressed;
	}

	void to_json(nlohmann::json& j, const texture_cook_config_t& c)
	{
		j["payload_type"]	  = c.payload_type;
		j["generate_mipmaps"] = c.generate_mipmaps;
		j["is_linear"]		  = c.is_linear;
	}

	void from_json(const nlohmann::json& j, texture_cook_config_t& c)
	{
		c.payload_type	   = j.value<texture_cook_payload_type_e>("payload_type", texture_cook_payload_type_e::uncompressed);
		c.generate_mipmaps = j.value<bool>("generate_mipmaps", false);
		c.is_linear		   = j.value<bool>("is_linear", false);
	}
}
