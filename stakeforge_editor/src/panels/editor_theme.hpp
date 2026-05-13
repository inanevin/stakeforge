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

#include <sfg/common/hashing.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct editor_theme_t
	{
		inline static editor_theme_t& get()
		{
			static editor_theme_t instance;
			return instance;
		}

		vec4f_t color_bg0 = color_t::from255(3.0f, 3.0f, 3.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_bg1 = color_t::from255(6.0f, 6.0f, 6.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_bg2 = color_t::from255(12.0f, 12.0f, 12.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_bg3 = color_t::from255(18.0f, 18.0f, 18.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_bg4 = color_t::from255(42.0f, 42.0f, 42.0f, 255.0f).srgb_to_linear().to_vector();

		vec4f_t color_fg0 = color_t::from255(107.0f, 107.0f, 107.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg1 = color_t::from255(143.0f, 143.0f, 143.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg2 = color_t::from255(179.0f, 179.0f, 179.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg3 = color_t::from255(214.0f, 214.0f, 214.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg4 = color_t::from255(245.0f, 245.0f, 245.0f, 255.0f).srgb_to_linear().to_vector();

		vec4f_t color_divider_dark	= color_bg0;
		vec4f_t color_divider_light = color_fg3;

		vec4f_t color_accent0	  = color_t::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent0_dim = color_t::from255(151.0f, 0.0f, 119.0f, 125.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent1	  = color_t::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent1_dim = color_t::from255(7, 131, 214, 125.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent2	  = color_t::from255(255.0f, 102.0f, 0.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent2_dim = color_t::from255(255.0f, 102.0f, 0.0f, 125.0f).srgb_to_linear().to_vector();
		vec4f_t color_highlight	  = color_t::from255(245.0f, 194.0f, 82.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent_warn = color_t::from255(245.0f, 194.0f, 82.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent_err  = color_t::from255(214.0f, 65.0f, 57.0f, 255.0f).srgb_to_linear().to_vector();

		sid_t font_default		= "editor/fonts/IBMPlexSans/IBMPlexSans-Regular.ttf"_hs;
		sid_t font_default_mono = "editor/fonts/IBMPlex-Mono/IBMPlexMono-Regular.ttf"_hs;
		sid_t font_sfg			= "editor/fonts/Orbitron/static/Orbitron-Bold.ttf"_hs;
		sid_t font_title		= "editor/fonts/Play/Play-Regular.ttf"_hs;
		sid_t font_title_bold	= "editor/fonts/Play/Play-Bold.ttf"_hs;
		sid_t font_icons		= "editor/fonts/icons.ttf"_hs;
		sid_t shader_glitch_lcd = "editor/shaders/editor_ui_text_lcd_glitch.hlsl"_hs;

		f32 aa_thickness			 = 2.0f;
		f32 text_default_px_size	 = 12.0f;
		f32 text_small_px_size		 = 10.0f;
		f32 text_big_title_px_size	 = 24.0f;
		f32 text_med_title_px_size	 = 14.0f;
		f32 text_small_title_px_size = 10.0f;
		f32 icon_default_px_size	 = 10.0f;
		f32 item_height				 = 24.0f;
		f32 item_spacing			 = 8.0f;
		f32 indent_horizontal		 = 8.0f;
		f32 margin_horizontal		 = 8.0f;
		f32 margin_vertical			 = 4.0f;
		f32 outline_thickness		 = 2.0f;
		f32 divider_thickness		 = 1.0f;
	};
}
