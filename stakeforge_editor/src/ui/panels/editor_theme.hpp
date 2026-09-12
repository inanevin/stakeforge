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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/color_utils.hpp>
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

		vec4f_t color_frame_dark   = color_utils_t::srgb_to_linear(color_t::from255(5, 5, 5, 255.0f)).to_vector();
		vec4f_t color_frame		   = color_utils_t::srgb_to_linear(color_t::from255(14, 14, 14, 255.0f)).to_vector();
		vec4f_t color_frame_light  = color_utils_t::srgb_to_linear(color_t::from255(20, 20, 20, 255.0f)).to_vector();
		vec4f_t color_panel		   = color_utils_t::srgb_to_linear(color_t::from255(28, 28, 28, 255.0f)).to_vector();
		vec4f_t color_panel_light  = color_utils_t::srgb_to_linear(color_t::from255(38.0f, 38.0f, 38.0f, 255.0f)).to_vector();
		vec4f_t color_panel_light1 = color_utils_t::srgb_to_linear(color_t::from255(55.0f, 55.0f, 55.0f, 255.0f)).to_vector();
		vec4f_t color_panel_light2 = color_utils_t::srgb_to_linear(color_t::from255(80.0f, 80.0f, 80.0f, 255.0f)).to_vector();
		vec4f_t color_light		   = color_utils_t::srgb_to_linear(color_t::from255(48.0f, 48.0f, 48.0f, 255.0f)).to_vector();

		vec4f_t color_text2			= color_utils_t::srgb_to_linear(color_t::from255(107.0f, 107.0f, 107.0f, 255.0f)).to_vector();
		vec4f_t color_text1			= color_utils_t::srgb_to_linear(color_t::from255(143.0f, 143.0f, 143.0f, 255.0f)).to_vector();
		vec4f_t color_text0			= color_utils_t::srgb_to_linear(color_t::from255(218.0f, 218.0f, 218.0f, 255.0f)).to_vector();
		vec4f_t color_text_disabled = color_text2;

		vec4f_t color_divider_dark	= color_frame;
		vec4f_t color_editor_border = color_utils_t::srgb_to_linear(color_t::from255(4, 4, 4, 255.0f)).to_vector();

		vec4f_t color_accent0_light	   = color_utils_t::srgb_to_linear(color_t::from255(180.0f, 0.0f, 119.0f, 255.0f)).to_vector();
		vec4f_t color_accent0		   = color_utils_t::srgb_to_linear(color_t::from255(151.0f, 0.0f, 119.0f, 255.0f)).to_vector();
		vec4f_t color_accent0_dim	   = color_utils_t::srgb_to_linear(color_t::from255(91.0f, 0.0f, 72.0f, 255.0f)).to_vector();
		vec4f_t color_accent1		   = color_utils_t::srgb_to_linear(color_t::from255(90, 190, 255, 255.0f)).to_vector();
		vec4f_t color_accent1_dim	   = color_utils_t::srgb_to_linear(color_t::from255(7, 131, 214, 200.0f)).to_vector();
		vec4f_t color_accent_green	   = color_utils_t::srgb_to_linear(color_t::from255(90.0f, 210.0f, 125.0f, 255.0f)).to_vector();
		vec4f_t color_accent_green_dim = color_utils_t::srgb_to_linear(color_t::from255(90.0f, 210.0f, 125.0f, 100.0f)).to_vector();
		vec4f_t color_accent2		   = color_utils_t::srgb_to_linear(color_t::from255(255.0f, 102.0f, 0.0f, 255.0f)).to_vector();
		vec4f_t color_accent2_dim	   = color_utils_t::srgb_to_linear(color_t::from255(255.0f, 102.0f, 0.0f, 125.0f)).to_vector();
		vec4f_t color_highlight		   = color_utils_t::srgb_to_linear(color_t::from255(245.0f, 194.0f, 82.0f, 255.0f)).to_vector();
		vec4f_t color_accent_warn	   = color_utils_t::srgb_to_linear(color_t::from255(245.0f, 194.0f, 82.0f, 255.0f)).to_vector();
		vec4f_t color_accent_err	   = color_utils_t::srgb_to_linear(color_t::from255(214.0f, 65.0f, 57.0f, 255.0f)).to_vector();
		vec4f_t color_outline		   = color_utils_t::srgb_to_linear(color_t::from255(14.0f, 14.0f, 14.0f, 255.0f)).to_vector();
		vec4f_t color_outline_light	   = color_utils_t::srgb_to_linear(color_t::from255(45.f, 45.f, 45.f, 255.0f)).to_vector();

		sid_t font_default		 = "editor/resource_pack/fonts/IBMPlexSans/IBMPlexSans-Regular.ttf"_hs;
		sid_t font_default_mono	 = "editor/resource_pack/fonts/IBMPlex-Mono/IBMPlexMono-Regular.ttf"_hs;
		sid_t font_sfg			 = "editor/resource_pack/fonts/Orbitron/static/Orbitron-Bold.ttf"_hs;
		sid_t font_title		 = "editor/resource_pack/fonts/Play/Play-Regular.ttf"_hs;
		sid_t font_title_bold	 = "editor/resource_pack/fonts/Play/Play-Bold.ttf"_hs;
		sid_t font_icons		 = "editor/resource_pack/fonts/icons.ttf"_hs;
		sid_t shader_glitch_lcd	 = "editor/resource_pack/shaders/editor_ui_text_lcd_glitch.hlsl"_hs;
		sid_t shader_glitch_rect = "editor/resource_pack/shaders/editor_ui_glitch_rect.hlsl"_hs;

		f32 aa_thickness			 = 2.0f;
		f32 text_big_px_size		 = 15.0f;
		f32 text_default_px_size	 = 12.0f;
		f32 text_small_px_size		 = 10.0f;
		f32 text_med_title_px_size	 = 14.0f;
		f32 text_small_title_px_size = 10.0f;
		f32 icon_default_px_size	 = 12.0f;
		f32 item_area_height		 = 28.0f;
		f32 item_height				 = 20.0f;
		f32 item_width				 = item_height * 5.0f;
		f32 item_spacing			 = 6.0f;
		f32 item_rounding			 = 2.0f;
		f32 indent_horizontal		 = 8.0f;
		f32 margin_horizontal		 = 8.0f;
		f32 margin_vertical			 = 4.0f;
		f32 outline_thickness		 = 1.0f;
		f32 divider_thickness		 = 1.0f;
		f32 border_thickness		 = 2.0f;
	};
}
