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

namespace sfg
{
	struct window_runtime_t;
	enum class window_style_e : u8;

	void set_script_api_platform_window_runtime(window_runtime_t* window);
	void reset_script_api_platform_cursor_state();
	void api_platform_set_cursor_visible(u8 visible);
	void api_platform_lock_cursor(u8 locked);
	void api_platform_set_window_size(u16 width, u16 height);
	void api_platform_set_window_style(window_style_e style);

	struct script_api_platform_t
	{
		u32 size									   = 0;
		u32 version									   = 0;
		void (*set_cursor_visible)(u8 visible)		   = nullptr;
		void (*lock_cursor)(u8 locked)				   = nullptr;
		void (*set_window_size)(u16 width, u16 height) = nullptr;
		void (*set_window_style)(window_style_e style) = nullptr;
	};

	const script_api_platform_t& get_script_api_platform();
}
