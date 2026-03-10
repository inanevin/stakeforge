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

#include "editor_theme.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "math/color.hpp"
#include <vendor/nhlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

namespace SFG
{
	f32 editor_theme::UI_SCALING = 1.0f;

	void to_json(nlohmann::json& j, const editor_theme& t)
	{
		j["root_rounding"]					= t.root_rounding;
		j["root_spacing"]					= t.root_spacing;
		j["outer_margin"]					= t.outer_margin;
		j["item_spacing"]					= t.item_spacing;
		j["row_spacing"]					= t.row_spacing;
		j["title_line_width"]				= t.title_line_width;
		j["title_line_height"]				= t.title_line_height;
		j["item_height"]					= t.item_height;
		j["row_height"]						= t.row_height;
		j["table_cell_height"]				= t.table_cell_height;
		j["property_cell_div"]				= t.property_cell_div;
		j["seperator_thickness"]			= t.seperator_thickness;
		j["area_rounding"]					= t.area_rounding;
		j["scroll_thickness"]				= t.scroll_thickness;
		j["scroll_rounding"]				= t.scroll_rounding;
		j["inner_margin"]					= t.inner_margin;
		j["frame_thickness"]				= t.frame_thickness;
		j["frame_rounding"]					= t.frame_rounding;
		j["context_menu_outline_thickness"] = t.context_menu_outline_thickness;

		j["col_title_line_start"]	   = t.col_title_line_start;
		j["col_title_line_end"]		   = t.col_title_line_end;
		j["col_hyperlink"]			   = t.col_hyperlink;
		j["col_accent"]				   = t.col_accent;
		j["col_accent_dim"]			   = t.col_accent_dim;
		j["col_accent_second"]		   = t.col_accent_second;
		j["col_accent_second_dim"]	   = t.col_accent_second_dim;
		j["col_accent_third"]		   = t.col_accent_third;
		j["col_accent_third_dim"]	   = t.col_accent_third_dim;
		j["col_highlight"]			   = t.col_highlight;
		j["col_highlight_transparent"] = t.col_highlight_transparent;
		j["col_scroll_bar"]			   = t.col_scroll_bar;
		j["col_scroll_bar_bg"]		   = t.col_scroll_bar_bg;
		j["col_title"]				   = t.col_title;
		j["col_text"]				   = t.col_text;
		j["col_text_dim"]			   = t.col_text_dim;
		j["col_light"]				   = t.col_light;
		j["col_frame_bg"]			   = t.col_frame_bg;
		j["col_area_bg"]			   = t.col_area_bg;
		j["col_root"]				   = t.col_root;
		j["col_button"]				   = t.col_button;
		j["col_button_silent"]		   = t.col_button_silent;
		j["col_button_hover"]		   = t.col_button_hover;
		j["col_button_press"]		   = t.col_button_press;
		j["col_frame_outline"]		   = t.col_frame_outline;
		j["col_context_menu_outline"]  = t.col_context_menu_outline;

		j["color_axis_x"] = t.color_axis_x;
		j["color_axis_y"] = t.color_axis_y;
		j["color_axis_z"] = t.color_axis_z;
	}

	void from_json(const nlohmann::json& j, editor_theme& s)
	{
		s.root_rounding					 = j.value<f32>("root_rounding", 0.0f);
		s.root_spacing					 = j.value<f32>("root_spacing", 0.0f);
		s.outer_margin					 = j.value<f32>("outer_margin", 0.0f);
		s.item_spacing					 = j.value<f32>("item_spacing", 0.0f);
		s.row_spacing					 = j.value<f32>("row_spacing", 0.0f);
		s.title_line_width				 = j.value<f32>("title_line_width", 0.0f);
		s.title_line_height				 = j.value<f32>("title_line_height", 0.0f);
		s.item_height					 = j.value<f32>("item_height", 0.0f);
		s.row_height					 = j.value<f32>("row_height", 0.0f);
		s.table_cell_height				 = j.value<f32>("table_cell_height", 0.0f);
		s.property_cell_div				 = j.value<f32>("property_cell_div", 0.0f);
		s.seperator_thickness			 = j.value<f32>("seperator_thickness", 0.0f);
		s.area_rounding					 = j.value<f32>("area_rounding", 0.0f);
		s.scroll_thickness				 = j.value<f32>("scroll_thickness", 0.0f);
		s.scroll_rounding				 = j.value<f32>("scroll_rounding", 0.0f);
		s.inner_margin					 = j.value<f32>("inner_margin", 0.0f);
		s.frame_thickness				 = j.value<f32>("frame_thickness", 0.0f);
		s.frame_rounding				 = j.value<f32>("frame_rounding", 0.0f);
		s.context_menu_outline_thickness = j.value<f32>("context_menu_outline_thickness", 0.0f);

		s.col_title_line_start		= j.value<vector4>("col_title_line_start", vector4::zero);
		s.col_title_line_end		= j.value<vector4>("col_title_line_end", vector4::zero);
		s.col_hyperlink				= j.value<vector4>("col_hyperlink", vector4::zero);
		s.col_accent				= j.value<vector4>("col_accent", vector4::zero);
		s.col_accent_dim			= j.value<vector4>("col_accent_dim", vector4::zero);
		s.col_accent_second			= j.value<vector4>("col_accent_second", vector4::zero);
		s.col_accent_second_dim		= j.value<vector4>("col_accent_second_dim", vector4::zero);
		s.col_accent_third_dim		= j.value<vector4>("col_accent_third_dim", vector4::zero);
		s.col_accent_third			= j.value<vector4>("col_accent_third", vector4::zero);
		s.col_highlight				= j.value<vector4>("col_highlight", vector4::zero);
		s.col_highlight_transparent = j.value<vector4>("col_highlight_transparent", vector4::zero);
		s.col_scroll_bar			= j.value<vector4>("col_scroll_bar", vector4::zero);
		s.col_scroll_bar_bg			= j.value<vector4>("col_scroll_bar_bg", vector4::zero);
		s.col_title					= j.value<vector4>("col_title", vector4::zero);
		s.col_text					= j.value<vector4>("col_text", vector4::zero);
		s.col_text_dim				= j.value<vector4>("col_text_dim", vector4::zero);
		s.col_light					= j.value<vector4>("col_light", vector4::zero);
		s.col_frame_bg				= j.value<vector4>("col_frame_bg", vector4::zero);
		s.col_area_bg				= j.value<vector4>("col_area_bg", vector4::zero);
		s.col_root					= j.value<vector4>("col_root", vector4::zero);
		s.col_button				= j.value<vector4>("col_button", vector4::zero);
		s.col_button_silent			= j.value<vector4>("col_button_silent", vector4::zero);
		s.col_button_hover			= j.value<vector4>("col_button_hover", vector4::zero);
		s.col_button_press			= j.value<vector4>("col_button_press", vector4::zero);
		s.col_frame_outline			= j.value<vector4>("col_frame_outline", vector4::zero);
		s.col_context_menu_outline	= j.value<vector4>("col_context_menu_outline", vector4::zero);

		s.color_axis_x = j.value<vector4>("color_axis_x", vector4::zero);
		s.color_axis_y = j.value<vector4>("color_axis_y", vector4::zero);
		s.color_axis_z = j.value<vector4>("color_axis_z", vector4::zero);
	}

	void editor_theme::init_defaults()
	{
		col_accent			  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_accent_dim		  = color::from255(91.0f, 0.0f, 72.0f, 225.0f).srgb_to_linear().to_vector();
		col_accent_second	  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();
		col_accent_second_dim = color::from255(7, 131, 214, 150.0f).srgb_to_linear().to_vector();
		col_accent_third	  = color::from255(255.0f, 102.0f, 0.0f, 255.0f).srgb_to_linear().to_vector();
		col_accent_third_dim  = color::from255(255.0f, 102.0f, 0.0f, 150.0f).srgb_to_linear().to_vector();
		col_title_line_start  = color::from255(91.0f, 0.0f, 72.0f, 0.0f).srgb_to_linear().to_vector();
		col_title_line_end	  = color::from255(151.0f, 0.0f, 119.0f, 255.0f).srgb_to_linear().to_vector();
		col_hyperlink		  = color::from255(7, 131, 214, 255.0f).srgb_to_linear().to_vector();

		col_highlight				= col_accent_second;
		col_highlight_transparent	= col_accent_second;
		col_highlight_transparent.w = 0.5f;

		col_title	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text	 = color::from255(180, 180, 180, 255).srgb_to_linear().to_vector();
		col_text_dim = color::from255(130, 130, 130, 255).srgb_to_linear().to_vector();
		col_light	 = color::from255(80, 80, 80, 255).srgb_to_linear().to_vector();
		col_frame_bg = color::from255(4, 4, 4, 255).srgb_to_linear().to_vector();
		col_area_bg	 = color::from255(15, 15, 15, 255).srgb_to_linear().to_vector();
		col_root	 = color::from255(28, 28, 28, 255).srgb_to_linear().to_vector();

		col_scroll_bar			 = col_accent;
		col_scroll_bar_bg		 = col_frame_bg;
		col_button				 = color::from255(28, 28, 28, 255).srgb_to_linear().to_vector();
		col_button_silent		 = color::from255(28, 28, 28, 125).srgb_to_linear().to_vector();
		col_button_hover		 = col_accent_second;
		col_button_press		 = col_accent_second_dim;
		col_frame_outline		 = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();
		col_context_menu_outline = color::from255(60, 60, 60, 255).srgb_to_linear().to_vector();

		root_rounding				   = 6.0f;
		outer_margin				   = 8;
		item_spacing				   = 5;
		root_spacing				   = 6;
		row_spacing					   = 6;
		row_height					   = 24;
		title_line_width			   = 0.8f;
		title_line_height			   = 2;
		item_height					   = 20;
		table_cell_height			   = 10;
		seperator_thickness			   = 2;
		property_cell_div			   = 0.3f;
		area_rounding				   = 8.0f;
		scroll_thickness			   = 8;
		scroll_rounding				   = 8.0f;
		inner_margin				   = 4;
		frame_thickness				   = 1;
		frame_rounding				   = 2.0f;
		context_menu_outline_thickness = 2.0f;
	}

	void editor_theme::apply_scaling()
	{
		const f32 f = UI_SCALING;
		outer_margin *= f;
		item_spacing *= f;
		root_spacing *= f;
		row_spacing *= f;
		row_height *= f;
		title_line_height *= f;
		item_height *= f;
		table_cell_height *= f;
		seperator_thickness *= f;
		scroll_thickness *= f;
		inner_margin *= f;
		frame_thickness *= f;
		context_menu_outline_thickness *= f;
	}

	void editor_theme::init(const char* base_directory)
	{
		const string last_path = string(base_directory) + "editor_theme.stksettings";
		init_defaults();

		// if (file_system::exists(last_path.c_str()))
		// {
		// 	load(last_path.c_str());
		// }
		// else
		// {
		// 	init_defaults();
		// 	save(last_path.c_str());
		// }
		_last_path = last_path;
	}

	bool editor_theme::load(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("file doesn't exists: {0}", path);
			return false;
		}

		try
		{
			std::ifstream f(path);
			editor_theme  st = json::parse(f);
			*this			 = st;
			f.close();
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading editor_theme: {0}", e.what());
			return false;
		}

		return true;
	}

	bool editor_theme::save(const char* path)
	{
		json j = *this;

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
			return true;
		}

		SFG_ERR("failed while writing json! {0}", path);
		return false;
	}

}
