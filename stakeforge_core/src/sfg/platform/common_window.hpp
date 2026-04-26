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
#include "common/size_definitions.hpp"
#include "math/vec2i16.hpp"
#include "math/vec2u16.hpp"
#include "data/bitmask.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace sfg
{
	struct window_event_t;
	typedef void (*window_event_callback)(void* hwnd, const window_event_t& ev, void* user_data);

	struct monitor_info_t
	{
		vec2i16_t position	  = vec2i16_t::zero;
		vec2u16_t size		  = vec2u16_t::zero;
		vec2u16_t work_size	  = vec2u16_t::zero;
		u64		  device_hash = 0;
		u32		  dpi		  = 0;
		f32		  dpi_scale	  = 0.0f;
		bool	  is_primary  = false;
	};

	enum class window_style_t : u8
	{
		app_window,
		borderless,
	};

	enum class window_cursor_confinement_t : u8
	{
		none,
		window,
		pointer,
	};

	enum window_event_flags
	{
		wef_high_freq = 1 << 0,
	};

	enum class window_cursor_state_t : u8
	{
		arrow,
		hand,
		resize_hr,
		resize_vt,
		resize_nwse,
		resize_nesw,
		caret,
	};
	enum class window_event_type_t : u8
	{
		key = 0,
		mouse,
		wheel,
		delta,
		focus,
		display_change,
		resize,
		repos,
	};

	enum class window_event_sub_type_t : u8
	{
		press,
		release,
		repeat,
	};

	struct window_event_t
	{
		vec2i16_t				value = vec2i16_t::zero;
		u16						button;
		window_event_type_t		type	 = window_event_type_t::key;
		window_event_sub_type_t sub_type = window_event_sub_type_t::press;
		bitmask_t<u8>			flags	 = 0;
	};

	enum class window_runtime_flags_t : u8
	{
		has_focus			 = 1 << 0,
		close_requested		 = 1 << 1,
		high_frequency_input = 1 << 2,
		minimized			 = 1 << 3,
	};

	struct window_runtime_t
	{
		monitor_info_t		  monitor_info			   = {};
		window_event_callback event_callback		   = nullptr;
		void*				  event_callback_user_data = nullptr;
		void*				  window_handle			   = nullptr;
		void*				  platform_handle		   = nullptr;
		vec2i16_t			  pos					   = vec2i16_t::zero;
		vec2u16_t			  size					   = vec2u16_t::zero;
		vec2u16_t			  true_size				   = vec2u16_t::zero;
		vec2i16_t			  mouse_position_abs	   = vec2i16_t::zero;
		vec2i16_t			  mouse_position		   = vec2i16_t::zero;
		gfx_swapchain_handle  swapchain				   = {};
		window_style_t		  style					   = window_style_t::app_window;
		bitmask_t<u8>		  flags					   = 0;

		inline bool has_flag(window_runtime_flags_t flag) const
		{
			return flags.is_set(static_cast<u8>(flag));
		}

		inline void set_flag(window_runtime_flags_t flag, bool is_set = true)
		{
			flags.set(static_cast<u8>(flag), is_set);
		}

		inline void remove_flag(window_runtime_flags_t flag)
		{
			flags.remove(static_cast<u8>(flag));
		}
	};

}
