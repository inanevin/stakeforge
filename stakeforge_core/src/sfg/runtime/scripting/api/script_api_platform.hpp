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

namespace sfg
{
	struct window_runtime_t;
	enum class window_style_e : u8;

	enum class script_cursor_lock_mode_e : u8
	{
		none,
		current_position,
		center,
	};

	typedef void (*script_api_platform_lock_cursor_fn)(script_cursor_lock_mode_e mode);

	extern bool g_window_api_enabled;

	void set_script_api_platform_window_runtime(window_runtime_t* window);
	void set_script_api_platform_lock_cursor_callback(script_api_platform_lock_cursor_fn lock_cursor);
	void reset_script_api_platform_cursor_state();
	void api_platform_set_cursor_visible(u8 visible);
	void api_platform_lock_cursor(script_cursor_lock_mode_e mode);
	void api_platform_set_window_size(u16 width, u16 height);
	void api_platform_set_window_style(window_style_e style);

	struct script_api_platform_t
	{
		u32 size											= 0;
		u32 version											= 0;
		void (*set_cursor_visible)(u8 visible)				= nullptr;
		void (*lock_cursor)(script_cursor_lock_mode_e mode) = nullptr;
		void (*set_window_size)(u16 width, u16 height)		= nullptr;
		void (*set_window_style)(window_style_e style)		= nullptr;
	};

	const script_api_platform_t& get_script_api_platform();
}
