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
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	struct editor_theme_t;

	using editor_action_menu_command_fn		 = void (*)(u16 command, void* user_data);
	using editor_action_menu_toggle_query_fn = bool (*)(void* user_data);
	using editor_action_menu_toggle_fn		 = void (*)(u16 command, bool value, void* user_data);
	using editor_action_menu_closed_fn		 = void (*)(void* user_data);

	enum class editor_action_menu_row_kind_e : u8
	{
		item,
		title,
		toggle,
	};

	struct editor_action_menu_row_desc_t
	{
		const char*							 text			  = nullptr;
		const char*							 shortcut		  = nullptr;
		const char*							 icon			  = nullptr;
		editor_action_menu_toggle_query_fn	 toggle_query	  = nullptr;
		editor_action_menu_toggle_fn		 toggle_callback  = nullptr;
		void*								 toggle_user_data = nullptr;
		bool*								 toggle_value	  = nullptr;
		const editor_action_menu_row_desc_t* children		  = nullptr;
		vec4f_t								 icon_color		  = {1, 1, 1, 1};
		u16									 child_count	  = 0;
		u16									 command		  = 0;
		editor_action_menu_row_kind_e		 kind			  = editor_action_menu_row_kind_e::item;
		bool								 has_icon_color	  = false;
		bool								 close_on_toggle  = false;
		bool								 disabled		  = false;
	};

	struct editor_action_menu_style_t
	{
		vec4f_t dropdown_color		 = {0, 0, 0, 0};
		vec4f_t hover_color			 = {0, 0, 0, 0};
		vec4f_t press_color			 = {0, 0, 0, 0};
		vec4f_t text_color			 = {1, 1, 1, 1};
		vec4f_t shortcut_color		 = {1, 1, 1, 1};
		vec4f_t disabled_text_color	 = {1, 1, 1, 1};
		vec4f_t title_color			 = {1, 1, 1, 1};
		vec4f_t title_line_color	 = {1, 1, 1, 1};
		vec4f_t icon_color			 = {1, 1, 1, 1};
		f32		min_width			 = 72.0f;
		f32		row_height			 = 24.0f;
		f32		text_size			 = 12.0f;
		f32		shortcut_size		 = 12.0f;
		f32		title_size			 = 10.0f;
		f32		title_line_thickness = 1.0f;
		f32		icon_size			 = 8.0f;
		f32		padding_x			 = 8.0f;
		f32		padding_y			 = 4.0f;
		f32		shortcut_gap		 = 32.0f;
		f32		title_gap			 = 8.0f;
	};

	editor_action_menu_style_t make_default_action_menu_style(const editor_theme_t& theme);
}
