/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "editor_text_rasterization.hpp"

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
