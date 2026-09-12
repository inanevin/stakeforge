/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "sprite_cook.hpp"
#include "ktx2_util.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	bool sprite_cooker::cook_from_file(const sprite_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		string_t extension = file_system_t::get_file_extension(full_path);
		string_util::to_lower(extension);

		if (extension != "png")
		{
			SFG_ERR("sprite source must be a PNG file: {0}", full_path);
			return false;
		}

		vec2u16_t size	   = vec2u16_t::zero;
		const u8  channels = 4;
		void*	  image	   = image_util_t::load_from_file_ch(full_path, size, 4);

		if (image == nullptr)
		{
			SFG_ERR("failed to load sprite source: {0}", full_path);
			return false;
		}

		const bool single_sprite = cfg.row_count == 0 && cfg.column_count == 0;

		if (!single_sprite && (cfg.row_count == 0 || cfg.column_count == 0))
		{
			SFG_ERR("sprite row and column counts must both be zero or both be nonzero: {0}", full_path);
			image_util_t::free(image);
			return false;
		}

		if (single_sprite && (cfg.padding_x != 0 || cfg.padding_y != 0))
		{
			SFG_ERR("sprite padding requires nonzero row and column counts: {0}", full_path);
			image_util_t::free(image);
			return false;
		}

		const u16 row_count		 = single_sprite ? 1 : cfg.row_count;
		const u16 column_count	 = single_sprite ? 1 : cfg.column_count;
		const u32 padding_width	 = static_cast<u32>(cfg.padding_x) * (column_count - 1);
		const u32 padding_height = static_cast<u32>(cfg.padding_y) * (row_count - 1);

		if (padding_width >= size.x || padding_height >= size.y)
		{
			SFG_ERR("sprite padding exceeds source dimensions: {0}", full_path);
			image_util_t::free(image);
			return false;
		}

		const u32 usable_width	= static_cast<u32>(size.x) - padding_width;
		const u32 usable_height = static_cast<u32>(size.y) - padding_height;

		if (usable_width % column_count != 0 || usable_height % row_count != 0)
		{
			SFG_ERR("sprite sheet cells do not divide evenly: {0}", full_path);
			image_util_t::free(image);
			return false;
		}

		const texture_buffer_t mip = {
			.pixels	   = static_cast<u8*>(image),
			.data_size = static_cast<u32>(size.x) * static_cast<u32>(size.y) * channels,
			.row_pitch = static_cast<u32>(size.x) * channels,
			.size	   = size,
			.bpp	   = channels,
		};
		ostream_t encoded = {};

		if (cfg.payload_type == sprite_payload_type_e::png)
		{
			if (!image_util_t::write_png(mip, channels, encoded))
			{
				SFG_ERR("failed to encode PNG sprite for {0}", full_path);
				image_util_t::free(image);
				return false;
			}
		}
		else
		{
			const span_t<const texture_buffer_t> mips = {
				.data = &mip,
				.size = 1,
			};

			if (!ktx2_util_t::encode_uastc(mips, false, cfg.ktx2_compression, full_path, encoded))
			{
				image_util_t::free(image);
				return false;
			}
		}

		image_util_t::free(image);

		SFG_ASSERT(encoded.get_size() <= UINT32_MAX);

		const sprite_header_t sprite_header = {
			.data_size		  = static_cast<u32>(encoded.get_size()),
			.size			  = size,
			.cell_size		  = vec2u16_t(static_cast<u16>(usable_width / column_count), static_cast<u16>(usable_height / row_count)),
			.padding		  = vec2u16_t(cfg.padding_x, cfg.padding_y),
			.row_count		  = row_count,
			.column_count	  = column_count,
			.payload_type	  = cfg.payload_type,
			.ktx2_compression = cfg.ktx2_compression,
		};

		out_header = {
			.type		 = resource_type_e::sprite,
			.magic		 = sprite_loader_t::WIRE_MAGIC,
			.version	 = sprite_loader_t::WIRE_VERSION,
			.source_tick = file_system_t::get_last_modified_ticks(full_path),
		};

		stream << sprite_header;
		stream.write_raw(encoded.get_raw(), encoded.get_size());
		return true;
	}

	sprite_cook_reflection_t::sprite_cook_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "sprite_payload_type_e",
			.fields =
				{
					{.name = "ktx2_uastc", .display_name = "KTX2 UASTC"},
					{.name = "png", .display_name = "PNG"},
				},
			.type_id   = type_id_t<sprite_payload_type_e>::value,
			.size	   = sizeof(sprite_payload_type_e),
			.alignment = alignof(sprite_payload_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name		  = "sprite_cook_config_t",
			.display_name = "Sprite Cook Config",
			.fields =
				{
					{.name = "row_count", .display_name = "Row Count", .tooltip = "Zero with zero columns imports a single sprite.", .offset = offsetof(sprite_cook_config_t, row_count), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name = "column_count", .display_name = "Column Count", .tooltip = "Zero with zero rows imports a single sprite.", .offset = offsetof(sprite_cook_config_t, column_count), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name = "padding_x", .display_name = "Padding X", .tooltip = "Horizontal pixel spacing between cells.", .offset = offsetof(sprite_cook_config_t, padding_x), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name = "padding_y", .display_name = "Padding Y", .tooltip = "Vertical pixel spacing between cells.", .offset = offsetof(sprite_cook_config_t, padding_y), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name		   = "ktx2_compression",
					 .display_name = "KTX2 Compression",
					 .sub_type_id  = type_id_t<texture_ktx2_compression_e>::value,
					 .offset	   = offsetof(sprite_cook_config_t, ktx2_compression),
					 .size		   = sizeof(texture_ktx2_compression_e),
					 .type		   = reflected_value_type_e::u8},
					{.name		   = "payload_type",
					 .display_name = "Payload Type",
					 .sub_type_id  = type_id_t<sprite_payload_type_e>::value,
					 .offset	   = offsetof(sprite_cook_config_t, payload_type),
					 .size		   = sizeof(sprite_payload_type_e),
					 .type		   = reflected_value_type_e::u8},
				},
			.type_id   = type_id_t<sprite_cook_config_t>::value,
			.size	   = sizeof(sprite_cook_config_t),
			.alignment = alignof(sprite_cook_config_t),
		});
	}
}
