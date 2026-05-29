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
#include <sfg/data/vector.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	struct monitor_info_t;
	struct vec2u16_t;
	struct vec2i16_t;
	struct window_runtime_t;
	enum class window_style_e : u8;
	enum class window_cursor_confinement_e : u8;
	enum class window_cursor_state_e : u8;

	enum character_mask
	{
		letter	   = 1 << 0,
		number	   = 1 << 1,
		separator  = 1 << 2,
		symbol	   = 1 << 4,
		whitespace = 1 << 5,
		control	   = 1 << 6,
		printable  = 1 << 7,
		op		   = 1 << 8,
		sign	   = 1 << 9,
	};

	class process
	{

	public:
		// -----------------------------------------------------------------------------
		// process lifetime
		// -----------------------------------------------------------------------------

		static void init();
		static void uninit();
		static void pump_os_messages();

		// -----------------------------------------------------------------------------
		// io
		// -----------------------------------------------------------------------------

		static void		open_url(const char* url);
		static bool		open_directory(const char* dir);
		static void		message_box(const char* title, const char* msg);
		static void		select_files(const char* title, const char* extension, vector_t<string_t>& out_files);
		static void		push_clipboard(const char* cp);
		static string_t select_folder(const char* title);
		static string_t select_file(const char* title, const char* extension);
		static string_t save_file(const char* title, const char* extension);
		static string_t get_clipboard();

		// -----------------------------------------------------------------------------
		// os query
		// -----------------------------------------------------------------------------

		static void		 get_all_monitors(vector_t<monitor_info_t>& out);
		static char		 get_character_from_key(u32 key);
		static u16		 get_character_mask_from_key(u32 key, char ch);
		static bool		 is_key_down(u16 key);
		static bool		 is_mouse_down(u16 button);
		static vec2i16_t get_cursor_position();

		// -----------------------------------------------------------------------------
		// window
		// -----------------------------------------------------------------------------

		static bool create_window(const char* title, const vec2i16_t& pos, const vec2u16_t& size, window_style_e window_style, f32 window_alpha, bool always_on_top, window_runtime_t& runtime);
		static void destroy_window(void* window_handle);
		static void set_window_runtime(void* window_handle, window_runtime_t& runtime);
		static void set_window_size(void* window, const vec2u16_t& size, window_style_e style);
		static void set_window_position(void* window, const vec2i16_t& pos);
		static void set_window_visible(void* window, bool visible);
		static void set_window_style(void* window, const vec2u16_t& size, window_style_e style);
		static void minimize_window(void* window);
		static void toggle_maximize_window(void* window);
		static void set_window_maximized(void* window, bool maximized);
		static void bring_to_front(void* window);
		static void set_cursor_confinement(void* window_handle, window_cursor_confinement_e conf);
		static void set_cursor_state(window_cursor_state_e state);
		static void set_cursor_visible(bool visible);

	private:
		static int s_prev_clip[4];
	};
}
