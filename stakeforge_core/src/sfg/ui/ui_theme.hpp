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

#pragma once

#include "common/size_definitions.hpp"
#include "math/vec4f.hpp"

namespace sfg::ui
{
	struct theme_t
	{
		vec4f_t color_panel_bg	   = {0.094f, 0.094f, 0.094f, 1.0f};
		vec4f_t color_item_bg	   = {0.012f, 0.012f, 0.012f, 1.0f};
		vec4f_t color_item_hover   = {0.125f, 0.125f, 0.125f, 1.0f};
		vec4f_t color_item_press   = {0.094f, 0.094f, 0.094f, 1.0f};
		vec4f_t color_item_outline = {0.165f, 0.165f, 0.165f, 1.0f};
		vec4f_t color_item_fg	   = {0.78f, 0.78f, 0.78f, 1.0f};
		vec4f_t color_focus		   = {0.40f, 0.60f, 1.00f, 1.0f};
		vec4f_t color_divider	   = {0.047f, 0.047f, 0.047f, 1.0f};

		f32 item_height		  = 24.0f;
		f32 item_spacing	  = 8.0f;
		f32 indent_horizontal = 8.0f;
		f32 margin_horizontal = 4.0f;
		f32 margin_vertical	  = 2.0f;
		f32 outline_thickness = 1.0f;
	};
}
