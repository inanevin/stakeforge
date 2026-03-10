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

#include "window_common.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"

#ifdef SFG_PLATFORM_WINDOWS
struct HWND__;
#endif
namespace SFG
{

	class window
	{
	private:
	public:
		typedef void (*event_callback)(const window_event& ev, void* user_data);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bool create(const char* title, u16 flags, const vector2i16& pos, const vector2ui16& size);
		void destroy();

		// -----------------------------------------------------------------------------
		// window api
		// -----------------------------------------------------------------------------

		void		set_position(const vector2i16& pos);
		void		maximize();
		void		set_size(const vector2ui16& size);
		void		set_style(window_flags flags);
		void		bring_to_front();
		void		confine_cursor(cursor_confinement conf);
		void		set_cursor_visible(bool vis);
		static void set_cursor_state(cursor_state cs);
		bool		is_maximized() const;
		static void query_all_monitors(vector<monitor_info>& out_info);
		static f32	get_wheel_delta();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		static bool is_key_down(u16 key);

		inline const vector2i16 get_position() const
		{
			return _position;
		}

		inline const vector2ui16 get_size() const
		{
			return _size;
		}
		inline const vector2ui16 get_true_size() const
		{
			return _true_size;
		}
		inline void* get_window_handle() const
		{
			return _window_handle;
		}

		inline void* get_platform_handle() const
		{
			return _platform_handle;
		}

		inline const bitmask<u16>& get_flags() const
		{
			return _flags;
		}

		inline const monitor_info& get_monitor_info() const
		{
			return _monitor_info;
		}

		inline void set_event_callback(event_callback cb, void* user_data)
		{
			_event_callback			  = cb;
			_event_callback_user_data = user_data;
		}

		inline const vector<string>& get_dropped_files() const
		{
			return _dropped_files;
		}

		inline void clear_dropped_files()
		{
			_dropped_files.resize(0);
		}

		inline void set_size_dirty(bool dirty)
		{
			_flags.set(window_flags::wf_size_dirty, dirty);
		}

		inline void set_pos_dirty(bool dirty)
		{
			_flags.set(window_flags::wf_pos_dirty, dirty);
		}

		inline const vector2i16& get_mouse_position() const
		{
			return _mouse_position;
		}

#ifdef SFG_PLATFORM_WINDOWS
		static __int64 wnd_proc(HWND__* hwnd, unsigned int msg, unsigned __int64 wParam, __int64 lParam);
#endif
		static f32 UI_SCALE;

	private:
		void add_event(const window_event& ev);

	private:
		event_callback		_event_callback			  = nullptr;
		void*				_event_callback_user_data = nullptr;
		monitor_info		_monitor_info			  = {};
		void*				_window_handle			  = nullptr;
		void*				_platform_handle		  = nullptr;
		int					_prev_confinement[4];
		vector2i16			_mouse_position		= vector2i16::zero;
		vector2i16			_mouse_position_abs = vector2i16::zero;
		vector2i16			_position			= vector2i16::zero;
		vector2ui16			_true_size			= vector2ui16::zero;
		vector2ui16			_size				= vector2ui16::zero;
		bitmask<u16>		_flags				= 0;
		vector<string>		_dropped_files;
		static u8			s_key_down_map[512];
		static cursor_state s_cursor_state;
	};

}
