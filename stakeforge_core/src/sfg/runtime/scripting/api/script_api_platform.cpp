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

#include "script_api_platform.hpp"

#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>

namespace sfg
{
	namespace
	{
		window_runtime_t* g_window_runtime = nullptr;
		bool			  g_cursor_locked  = false;
		bool			  g_cursor_visible = true;
	}

	void set_script_api_platform_window_runtime(window_runtime_t* window)
	{
		if (g_window_runtime != nullptr && g_window_runtime != window)
		{
			if (g_cursor_locked)
				process::set_cursor_confinement(g_window_runtime->window_handle, window_cursor_confinement_e::none);

			if (!g_cursor_visible)
				process::set_cursor_visible(true);

			g_cursor_locked	 = false;
			g_cursor_visible = true;
		}

		g_window_runtime = window;
	}

	void api_platform_set_cursor_visible(u8 visible)
	{
		if (g_window_runtime == nullptr)
			return;

		process::set_cursor_visible(visible != 0);
		g_cursor_visible = visible != 0;
	}

	void api_platform_lock_cursor(u8 locked)
	{
		if (g_window_runtime == nullptr)
			return;

		const bool is_locked = locked != 0;

		if (g_cursor_locked == is_locked)
			return;

		process::set_cursor_confinement(g_window_runtime->window_handle, is_locked ? window_cursor_confinement_e::pointer : window_cursor_confinement_e::none);
		g_cursor_locked = is_locked;
	}

	void api_platform_set_window_size(u16 width, u16 height)
	{
		if (g_window_runtime == nullptr)
			return;

		process::set_window_size(g_window_runtime->window_handle, {width, height}, g_window_runtime->style);
	}

	void api_platform_set_window_style(window_style_e style)
	{
		if (g_window_runtime == nullptr)
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
