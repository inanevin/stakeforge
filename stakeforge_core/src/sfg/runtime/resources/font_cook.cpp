// Copyright (c) 2025 Inan Evin

#include "font_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <sfg/vendor/stb/stb_truetype.h>

namespace sfg
{
	bool font_cooker::cook_from_file(const font_cook_config_t&, const char* full_path, ostream_t& stream)
	{
		char*  ttf_data = nullptr;
		size_t ttf_size = 0;
		file_system_t::read_file(full_path, ttf_data, ttf_size);
		if (ttf_data == nullptr || ttf_size == 0)
		{
			SFG_ERR("failed to read font file {0}", full_path);
			return false;
		}

		const size_t	  header_pos = stream.get_size();
		resource_header_t header	 = {
				.magic		  = font_loader_t::WIRE_MAGIC,
				.version	  = font_loader_t::WIRE_VERSION,
				.source_ticks = {file_system_t::get_last_modified_ticks(full_path)},
		};
		header.serialize(stream);

		stream << static_cast<u32>(ttf_size);
		stream.write_raw(reinterpret_cast<const u8*>(ttf_data), ttf_size);

		delete[] ttf_data;
		return true;
	}

	void from_json(const nlohmann::json&, font_cook_config_t& c)
	{
		c.reserved = 0;
	}
}
