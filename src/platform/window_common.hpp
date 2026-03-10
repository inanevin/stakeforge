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
#include "math/vector2i16.hpp"
#include "math/vector2ui16.hpp"
#include "data/bitmask.hpp"

namespace SFG
{
	struct monitor_info
	{
		vector2i16	position   = vector2i16::zero;
		vector2ui16 size	   = vector2ui16::zero;
		vector2ui16 work_size  = vector2ui16::zero;
		u32			dpi		   = 0;
		f32			dpi_scale  = 0.0f;
		bool		is_primary = false;
	};

	enum window_flags
	{
		wf_size_dirty			   = 1 << 0,
		wf_has_focus			   = 1 << 1,
		wf_high_freq			   = 1 << 2,
		wf_close_requested		   = 1 << 3,
		wf_style_windowed		   = 1 << 4,
		wf_style_borderless		   = 1 << 5,
		wf_cursor_hidden		   = 1 << 6,
		wf_cursor_confined_window  = 1 << 7,
		wf_cursor_confined_pointer = 1 << 8,
		wf_pos_dirty			   = 1 << 9,
	};

	enum window_event_flags
	{
		wef_high_freq = 1 << 0,
	};

	enum class cursor_state : u8
	{
		arrow,
		hand,
		resize_hr,
		resize_vt,
		resize_nwse,
		resize_nesw,
		caret,
	};
	enum class window_event_type : u8
	{
		key = 0,
		mouse,
		wheel,
		delta,
		focus,
		display_change,
	};

	enum class window_event_sub_type : u8
	{
		press,
		release,
		repeat,
	};

	enum class cursor_confinement : u8
	{
		none,
		window,
		pointer,
	};

	struct window_event
	{
		vector2i16			  value = vector2i16::zero;
		u16					  button;
		window_event_type	  type	   = window_event_type::key;
		window_event_sub_type sub_type = window_event_sub_type::press;
		bitmask<u8>			  flags	   = 0;
	};
}
