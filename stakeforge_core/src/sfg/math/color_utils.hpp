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

#pragma once

#include <cstddef>
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class color_t;

	class color_utils_t final
	{
	public:
		static color_t	lerp(const color_t& c1, const color_t& c2, f32 a);
		static color_t	from_hex(const string_t& hex);
		static string_t to_hex(const color_t& color_t);
		static void		to_hex(const color_t& color_t, char* out, size_t capacity);
		static color_t	hs_to_srgb(const color_t& color_t);
		static color_t	srgb_to_hsv(const color_t& color_t);
		static color_t	hsv_to_srgb(const color_t& color_t);
		static color_t	srgb_to_linear(const color_t& color_t);
		static color_t	linear_to_srgb(const color_t& color_t);
		static color_t	brighten(const color_t& color_t, f32 amt);
		static color_t	darken(const color_t& color_t, f32 amt);
	};

} // namespace sfg
