// Copyright (c) 2025 Inan Evin
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
		vec4f_t color_bg3 = color_t::from255(24.0f, 24.0f, 24.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_bg4 = color_t::from255(42.0f, 42.0f, 42.0f, 255.0f).srgb_to_linear().to_vector();

		vec4f_t color_fg0 = color_t::from255(107.0f, 107.0f, 107.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg1 = color_t::from255(143.0f, 143.0f, 143.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg2 = color_t::from255(179.0f, 179.0f, 179.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg3 = color_t::from255(214.0f, 214.0f, 214.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_fg4 = color_t::from255(245.0f, 245.0f, 245.0f, 255.0f).srgb_to_linear().to_vector();

		vec4f_t color_accent0	= color_t::from255(102.0f, 153.0f, 255.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_accent1	= color_t::from255(69.0f, 199.0f, 168.0f, 255.0f).srgb_to_linear().to_vector();
		vec4f_t color_highlight = color_t::from255(245.0f, 194.0f, 82.0f, 255.0f).srgb_to_linear().to_vector();

		sid_t font_default		= "editor/fonts/IBMPlexSans/IBMPlexSans-Regular.ttf"_hs;
		sid_t font_default_mono = "editor/fonts/IBMPlex-Mono/IBMPlexMono-Regular.ttf"_hs;
		sid_t font_big_title	= "editor/fonts/Orbitron/static/Orbitron-Bold.ttf"_hs;

		f32 text_default_px_size   = 16.0f;
		f32 text_big_title_px_size = 42.0f;
		f32 item_height			   = 28.0f;
		f32 item_spacing		   = 8.0f;
		f32 indent_horizontal	   = 8.0f;
		f32 margin_horizontal	   = 8.0f;
		f32 margin_vertical		   = 4.0f;
		f32 outline_thickness	   = 2.0f;
		f32 divider_thickness	   = 1.0f;
	};
}
