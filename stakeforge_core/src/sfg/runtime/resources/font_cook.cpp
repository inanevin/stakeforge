// Copyright (c) 2025 Inan Evin

#include "font_cook.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	bool font_cooker::cook_from_file(const font_cook_config_t&, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		char*  ttf_data = nullptr;
		size_t ttf_size = 0;
		file_system_t::read_file(full_path, ttf_data, ttf_size);
		if (ttf_data == nullptr || ttf_size == 0)
		{
			SFG_ERR("failed to read font file {0}", full_path);
			return false;
		}

		out_header = {
			.magic		 = font_loader_t::WIRE_MAGIC,
			.version	 = font_loader_t::WIRE_VERSION,
			.source_tick = file_system_t::get_last_modified_ticks(full_path),
		};

		ostream_t payload;
		payload << static_cast<u32>(ttf_size);
		payload.write_raw(reinterpret_cast<const u8*>(ttf_data), ttf_size);

		delete[] ttf_data;

		stream = compressor_t::compress(payload);
		if (stream.get_size() == 0)
			return false;

		return true;
	}

}

namespace sfg
{
	font_cook_config_reflection_t::font_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<font_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "reserved", .display_name = "Reserved", .type = reflected_value_type_e::u32, .offset = offsetof(font_cook_config_t, reserved), .size = sizeof(u32), .flags = reflected_field_flags_no_ui},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "font_cook_config_t",
			.type_id   = type_id_t<font_cook_config_t>::value,
			.size	   = sizeof(font_cook_config_t),
			.alignment = alignof(font_cook_config_t),
		});
	}
}
