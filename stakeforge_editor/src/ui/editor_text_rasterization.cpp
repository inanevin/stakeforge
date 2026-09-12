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
#include "ui/editor_text_rasterization.hpp"

namespace sfg
{
	ui::glyph_raster_mode_e editor_text_rasterization_t::_rasterization_type = ui::glyph_raster_mode_e::grayscale;

	bool editor_text_rasterization_t::is_subpixel_enabled()
	{
		return _rasterization_type == ui::glyph_raster_mode_e::lcd;
	}

	void editor_text_rasterization_t::set_subpixel_enabled(bool enabled)
	{
		set_rasterization_type(enabled ? ui::glyph_raster_mode_e::lcd : ui::glyph_raster_mode_e::grayscale);
	}

	ui::glyph_raster_mode_e editor_text_rasterization_t::get_rasterization_type()
	{
		return _rasterization_type;
	}

	void editor_text_rasterization_t::set_rasterization_type(ui::glyph_raster_mode_e rasterization_type)
	{
		_rasterization_type = rasterization_type;
	}
}
