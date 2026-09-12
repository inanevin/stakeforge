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

	struct process_execute_result_t
	{
		string_t output;
		i32		 exit_code = -1;
		bool	 started   = false;
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

		static void						open_url(const char* url);
		static void						open_file(const char* path);
		static void						open_file_in_vscode(const char* workspace_path, const char* path);
		static bool						open_directory(const char* dir);
		static void						message_box(const char* title, const char* msg);
		static void						select_files(const char* title, const char* extension, vector_t<string_t>& out_files);
		static void						push_clipboard(const char* cp);
		static process_execute_result_t execute(const char* command_line);
		static string_t					select_folder(const char* title);
		static string_t					select_file(const char* title, const char* extension);
		static string_t					save_file(const char* title, const char* extension);
		static string_t					get_clipboard();

		// -----------------------------------------------------------------------------
		// os query
		// -----------------------------------------------------------------------------

		static void					 get_all_monitors(vector_t<monitor_info_t>& out);
		static const monitor_info_t& find_primary_monitor(const vector_t<monitor_info_t>& monitors);
		static char					 get_character_from_key(u32 key);
		static u16					 get_character_mask_from_key(u32 key, char ch);
		static bool					 is_key_down(u16 key);
		static bool					 is_mouse_down(u16 button);
		static vec2i16_t			 get_cursor_position();
		static void					 set_cursor_position(void* window_handle, const vec2i16_t& position);

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
		static void set_cursor_confinement_position(void* window_handle, const vec2i16_t& position);
		static void set_cursor_state(window_cursor_state_e state);
		static void set_cursor_visible(void* window_handle, bool visible);
	};
}
