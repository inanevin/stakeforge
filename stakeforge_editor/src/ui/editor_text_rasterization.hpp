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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	class editor_app_t;

	class editor_text_rasterization_t final
	{
	public:
		editor_text_rasterization_t() = delete;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static bool					   is_subpixel_enabled();
		static void					   set_subpixel_enabled(bool enabled);
		static ui::glyph_raster_mode_e get_rasterization_type();

	private:
		friend class editor_app_t;

		static void set_rasterization_type(ui::glyph_raster_mode_e rasterization_type);

	private:
		static ui::glyph_raster_mode_e _rasterization_type;
	};
}
