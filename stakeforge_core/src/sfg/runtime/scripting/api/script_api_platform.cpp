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

#include "script_api_platform.hpp"

#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>

namespace sfg
{
	bool g_window_api_enabled = true;

	namespace
	{
		window_runtime_t*				   g_window_runtime = nullptr;
		script_api_platform_lock_cursor_fn g_lock_cursor	= nullptr;
	}

	void set_script_api_platform_window_runtime(window_runtime_t* window)
	{
		if (g_window_runtime != nullptr && g_window_runtime != window)
			process::set_cursor_visible(g_window_runtime->window_handle, true);

		g_window_runtime = window;
	}

	void set_script_api_platform_lock_cursor_callback(script_api_platform_lock_cursor_fn lock_cursor)
	{
		if (g_lock_cursor != nullptr && g_lock_cursor != lock_cursor)
			g_lock_cursor(script_cursor_lock_mode_e::none);

		g_lock_cursor = lock_cursor;
	}

	void reset_script_api_platform_cursor_state()
	{
		if (g_lock_cursor != nullptr)
			g_lock_cursor(script_cursor_lock_mode_e::none);

		if (g_window_runtime != nullptr)
			process::set_cursor_visible(g_window_runtime->window_handle, true);
	}

	void api_platform_set_cursor_visible(u8 visible)
	{
		if (g_window_runtime == nullptr)
			return;

		process::set_cursor_visible(g_window_runtime->window_handle, visible != 0);
	}

	void api_platform_lock_cursor(script_cursor_lock_mode_e mode)
	{
		if (g_lock_cursor != nullptr)
			g_lock_cursor(mode);
	}

	void api_platform_set_window_size(u16 width, u16 height)
	{
		if (!g_window_api_enabled || g_window_runtime == nullptr)
			return;

		process::set_window_size(g_window_runtime->window_handle, {width, height}, g_window_runtime->style);
	}

	void api_platform_set_window_style(window_style_e style)
	{
		if (!g_window_api_enabled || g_window_runtime == nullptr)
			return;

		process::set_window_style(g_window_runtime->window_handle, g_window_runtime->size, style);
	}

	const script_api_platform_t& get_script_api_platform()
	{
		static const script_api_platform_t api{
			.size				= static_cast<u32>(sizeof(script_api_platform_t)),
			.version			= 1,
			.set_cursor_visible = api_platform_set_cursor_visible,
			.lock_cursor		= api_platform_lock_cursor,
			.set_window_size	= api_platform_set_window_size,
			.set_window_style	= api_platform_set_window_style,
		};

		return api;
	}
}
