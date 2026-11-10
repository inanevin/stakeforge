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

#include "core/platform/executable_util.hpp"

#include "core/data/ostream.hpp"
#include "core/data/string_util.hpp"
#include "core/gfx/common/texture_buffer.hpp"
#include "core/gfx/util/image_util.hpp"
#include "core/io/log.hpp"
#include "core/memory/memory.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace sfg
{
	bool executable_util_t::set_icon(const char* executable_path, const char* image_path, string_t& diagnostics)
	{
		diagnostics.resize(0);

		vec2u16_t source_size = vec2u16_t::zero;
		u8* const source	  = static_cast<u8*>(image_util_t::load_from_file_ch(image_path, source_size, 4));

		if (source == nullptr)
		{
			diagnostics = string_t("Failed to read executable icon image: ") + image_path;
			return false;
		}

		if (source_size.x == 0 || source_size.y == 0)
		{
			image_util_t::free(source);
			diagnostics = string_t("Invalid executable icon image dimensions: ") + image_path;
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		// fit the image into each icon size without stretching it
		const u16	 sizes[]				  = {16, 24, 32, 48, 64, 128, 256};
		ostream_t	 images[std::size(sizes)] = {};
		vector_t<u8> resized_pixels			  = {};
		vector_t<u8> icon_pixels			  = {};

		resized_pixels.reserve(256 * 256 * 4);
		icon_pixels.reserve(256 * 256 * 4);

		for (size_t index = 0; index < std::size(sizes); ++index)
		{
			const u16		size		 = sizes[index];
			const u32		longest_side = std::max(source_size.x, source_size.y);
			const vec2u16_t resized_size{
				.x = static_cast<u16>(std::max(1u, static_cast<u32>(source_size.x) * size / longest_side)),
				.y = static_cast<u16>(std::max(1u, static_cast<u32>(source_size.y) * size / longest_side)),
			};

			resized_pixels.resize(static_cast<size_t>(resized_size.x) * resized_size.y * 4);

			const bool resized = image_util_t::resize_rgba8({.data = source, .size = static_cast<size_t>(source_size.x) * source_size.y * 4}, source_size, {.data = resized_pixels.data(), .size = resized_pixels.size()}, resized_size);

			if (!resized)
			{
				diagnostics = string_t("Failed to resize executable icon image: ") + image_path;
				break;
			}

			icon_pixels.resize(static_cast<size_t>(size) * size * 4);
			SFG_MEMSET(icon_pixels.data(), 0, icon_pixels.size());

			const u32 offset_x	= (size - resized_size.x) / 2;
			const u32 offset_y	= (size - resized_size.y) / 2;
			const u32 row_bytes = static_cast<u32>(resized_size.x) * 4;

			for (u32 row = 0; row < resized_size.y; ++row)
			{
				SFG_MEMCPY(icon_pixels.data() + ((row + offset_y) * size + offset_x) * 4, resized_pixels.data() + row * row_bytes, row_bytes);
			}

			const texture_buffer_t buffer{
				.pixels	   = icon_pixels.data(),
				.data_size = static_cast<u32>(icon_pixels.size()),
				.row_pitch = static_cast<u32>(size) * 4,
				.size	   = {.x = size, .y = size},
				.bpp	   = 4,
			};

			if (!image_util_t::write_png(buffer, 4, images[index]))
			{
				diagnostics = string_t("Failed to encode executable icon image: ") + image_path;
				break;
			}
		}

		image_util_t::free(source);

		if (!diagnostics.empty())
		{
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		// keep the executable's other resources and write the icon group
		const wstring_t path   = string_util::to_wstr(executable_path);
		HANDLE const	update = BeginUpdateResourceW(path.c_str(), FALSE);

		if (update == nullptr)
		{
			diagnostics = string_t("Failed to open executable for icon update: ") + executable_path + ", Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		ostream_t group = {};

		group << u16{0} << u16{1} << static_cast<u16>(std::size(sizes));

		for (size_t index = 0; index < std::size(sizes); ++index)
		{
			const u16 resource_id = static_cast<u16>(index + 1);
			const u8  dimension	  = sizes[index] == 256 ? 0 : static_cast<u8>(sizes[index]);

			group << dimension << dimension << u8{0} << u8{0} << u16{1} << u16{32};
			group << static_cast<u32>(images[index].get_size()) << resource_id;

			const bool written = UpdateResourceW(update, MAKEINTRESOURCEW(3), MAKEINTRESOURCEW(resource_id), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), images[index].get_raw(), static_cast<DWORD>(images[index].get_size())) != FALSE;

			if (!written)
			{
				diagnostics = string_t("Failed to write executable icon: ") + executable_path + ", Windows error " + std::to_string(GetLastError());
				break;
			}
		}

		if (diagnostics.empty())
		{
			const bool written = UpdateResourceW(update, MAKEINTRESOURCEW(14), MAKEINTRESOURCEW(ICON_RESOURCE_ID), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), group.get_raw(), static_cast<DWORD>(group.get_size())) != FALSE;

			if (!written)
				diagnostics = string_t("Failed to write executable icon group: ") + executable_path + ", Windows error " + std::to_string(GetLastError());
		}

		const bool discard = !diagnostics.empty();

		if (!EndUpdateResourceW(update, discard ? TRUE : FALSE))
			diagnostics += string_t("\nFailed to finish executable icon update: ") + executable_path + ", Windows error " + std::to_string(GetLastError());

		if (!diagnostics.empty())
		{
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		return true;
	}
}
