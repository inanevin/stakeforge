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
		editor_action_menu_row_kind_e		 kind			  = editor_action_menu_row_kind_e::item;
		const char*							 text			  = nullptr;
		const char*							 shortcut		  = nullptr;
		const char*							 icon			  = nullptr;
		vec4f_t								 icon_color		  = {1, 1, 1, 1};
		const editor_action_menu_row_desc_t* children		  = nullptr;
		u16									 child_count	  = 0;
		u16									 command		  = 0;
		bool*								 toggle_value	  = nullptr;
		editor_action_menu_toggle_query_fn	 toggle_query	  = nullptr;
		editor_action_menu_toggle_fn		 toggle_callback  = nullptr;
		void*								 toggle_user_data = nullptr;
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
