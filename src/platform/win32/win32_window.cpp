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

#include "platform/window.hpp"
#include "io/log.hpp"
#include "input/input_common.hpp"
#include "input/input_mappings.hpp"
#include "math/math.hpp"
#include <windowsx.h>
#include <hidusage.h>
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <string>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

namespace SFG
{
	u8			 window::s_key_down_map[512] = {};
	cursor_state window::s_cursor_state		 = cursor_state::arrow;
	f32			 window::UI_SCALE			 = 1.0f;

	namespace
	{

		static UINT map_numpad_vs_extended(USHORT sc, bool is_extended, UINT fallback_vk)
		{
			switch (sc)
			{
			case 0x47:
				return is_extended ? VK_HOME : VK_NUMPAD7;
			case 0x48:
				return is_extended ? VK_UP : VK_NUMPAD8;
			case 0x49:
				return is_extended ? VK_PRIOR : VK_NUMPAD9;
			case 0x4B:
				return is_extended ? VK_LEFT : VK_NUMPAD4;
			case 0x4C:
				return VK_NUMPAD5; // no extended version
			case 0x4D:
				return is_extended ? VK_RIGHT : VK_NUMPAD6;
			case 0x4F:
				return is_extended ? VK_END : VK_NUMPAD1;
			case 0x50:
				return is_extended ? VK_DOWN : VK_NUMPAD2;
			case 0x51:
				return is_extended ? VK_NEXT : VK_NUMPAD3;
			case 0x52:
				return is_extended ? VK_INSERT : VK_NUMPAD0;
			case 0x53:
				return is_extended ? VK_DELETE : VK_DECIMAL;
			default:
				return fallback_vk;
			}
		}

		auto composition_enabled() -> bool
		{
			BOOL composition_enabled = FALSE;
			bool success			 = ::DwmIsCompositionEnabled(&composition_enabled) == S_OK;
			return composition_enabled && success;
		}

		u32 get_style(u8 flags)
		{
			if (flags & window_flags::wf_style_windowed)
				return static_cast<u32>(WS_OVERLAPPEDWINDOW);
			else
			{
				return WS_POPUP | WS_VISIBLE;

				DWORD style = 0;
				if (composition_enabled())
					style = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
				else
					style = WS_POPUP | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

				return style;
			}
		}

		static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMon, HDC, LPRECT, LPARAM lParam)
		{
			auto* out = reinterpret_cast<vector<monitor_info>*>(lParam);

			MONITORINFO mi{};
			mi.cbSize = sizeof(mi);

			if (GetMonitorInfoW(hMon, &mi))
			{
				monitor_info info;

				UINT	dpiX, dpiY;
				HRESULT temp2	= GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
				info.size		= {static_cast<u16>(mi.rcMonitor.right - mi.rcMonitor.left), static_cast<u16>(mi.rcMonitor.bottom - mi.rcMonitor.top)};
				info.work_size	= {static_cast<u16>(mi.rcWork.right - mi.rcWork.left), static_cast<u16>(mi.rcWork.bottom - mi.rcWork.top)};
				info.position	= {static_cast<i16>(mi.rcWork.left), static_cast<i16>(mi.rcWork.top)};
				info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
				info.dpi		= dpiX;
				info.dpi_scale	= static_cast<f32>(dpiX) / 96.0f;

				out->push_back(info);
			}

			// continue enumeration
			return TRUE;
		}

		monitor_info fetch_monitor_info(HMONITOR monitor)
		{
			monitor_info info;

			MONITORINFOEX monitor_info;
			monitor_info.cbSize = sizeof(monitor_info);
			GetMonitorInfo(monitor, &monitor_info);

			UINT	dpiX, dpiY;
			HRESULT temp2	= GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
			info.size		= {static_cast<u16>(monitor_info.rcMonitor.right - monitor_info.rcMonitor.left), static_cast<u16>(monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top)};
			info.work_size	= {static_cast<u16>(monitor_info.rcWork.right - monitor_info.rcWork.left), static_cast<u16>(monitor_info.rcWork.bottom - monitor_info.rcWork.top)};
			info.position	= {static_cast<i16>(monitor_info.rcWork.left), static_cast<i16>(monitor_info.rcWork.top)};
			info.is_primary = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0;
			info.dpi		= dpiX;
			info.dpi_scale	= static_cast<f32>(dpiX) / 96.0f;
			return info;
		}

		f32 compute_ui_scale(const monitor_info& info)
		{
			f32 ui_scale = 1.0f;
			if (info.size.y < 1400)
				ui_scale = 0.75f;
			else if (info.size.y < 2000)
				ui_scale = 0.9f;
			return ui_scale * info.dpi_scale;
		}
	} // namespace

	LRESULT __stdcall window::wnd_proc(HWND__* hwnd, unsigned int msg, unsigned __int64 wParam, __int64 lParam)
	{
		window* wnd = reinterpret_cast<window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (!wnd)
			return DefWindowProcA(hwnd, msg, wParam, lParam);

		switch (msg)
		{
		case WM_SETCURSOR: {
			if (LOWORD(lParam) == HTCLIENT) // only client area
			{
				switch (s_cursor_state)
				{
				case cursor_state::arrow:
					::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
					return TRUE;
				case cursor_state::hand:
					::SetCursor(::LoadCursor(nullptr, IDC_HAND));
					return TRUE;
				case cursor_state::resize_hr:
					::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
					return TRUE;
				case cursor_state::resize_vt:
					::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
					return TRUE;
				case cursor_state::resize_nwse:
					::SetCursor(::LoadCursor(nullptr, IDC_SIZENWSE));
					return TRUE;
				case cursor_state::resize_nesw:
					::SetCursor(::LoadCursor(nullptr, IDC_SIZENESW));
					return TRUE;
				case cursor_state::caret:
					::SetCursor(::LoadCursor(nullptr, IDC_IBEAM));
					return TRUE;
				}
			}
			break;
		}
		case WM_DROPFILES: {
			HDROP hDrop = (HDROP)wParam;
			UINT  count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
			for (UINT i = 0; i < count; ++i)
			{
				UINT		 len = DragQueryFileW(hDrop, i, nullptr, 0);
				std::wstring wpath;
				wpath.resize(len);
				DragQueryFileW(hDrop, i, wpath.data(), len + 1);
				int needed = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
				if (needed > 0)
				{
					std::string path;
					path.resize((size_t)needed - 1);
					WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, path.data(), needed, nullptr, nullptr);
					wnd->_dropped_files.push_back(path.c_str());
				}
			}
			DragFinish(hDrop);
			return 0;
		}
		case WM_DPICHANGED:
		case WM_DISPLAYCHANGE: {
			wnd->_monitor_info	  = fetch_monitor_info(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
			window::UI_SCALE	  = compute_ui_scale(wnd->_monitor_info);
			const window_event ev = {
				.type = window_event_type::display_change,
			};
			wnd->add_event(ev);
			break;
		}
		case WM_CLOSE: {
			wnd->_flags.set(window_flags::wf_close_requested);
			return 0;
		}
		case WM_KILLFOCUS: {
			wnd->_flags.remove(wf_has_focus);

			const window_event ev = {
				.value = vec2i16(0, 0),
				.type  = window_event_type::focus,
			};

			wnd->add_event(ev);
			return 0;
		}
		case WM_SETFOCUS: {
			wnd->_flags.set(wf_has_focus);

			const window_event ev = {
				.value = vec2i16(1, 0),
				.type  = window_event_type::focus,
			};
			wnd->add_event(ev);

			return 0;
		}
		case WM_MOVE: {
			// const i32 x  = static_cast<i32>((short)LOWORD(lParam));
			// const i32 y  = static_cast<i32>((short)HIWORD(lParam));
			// wnd->_position = vec2i16(x, y);
			// wnd->_flags.set(window_flags::wf_pos_dirty);

			RECT r;
			GetWindowRect(hwnd, &r); // outer window (includes frame/caption)
			wnd->_position = vec2i16(r.left, r.top);
			wnd->_flags.set(window_flags::wf_pos_dirty);

			return 0;
		}
		case WM_SIZE: {

			const UINT width  = LOWORD(lParam);
			const UINT height = HIWORD(lParam);

			// RECT clientRect;
			// GetClientRect(hwnd, &clientRect);

			if (wnd->_size.x == width && wnd->_size.y == height)
				return 0;

			wnd->_size = vec2u16(static_cast<u16>(width), static_cast<u16>(height));
			wnd->_flags.set(wf_size_dirty);

			if (wnd->_flags.is_set(window_flags::wf_style_windowed))
			{
				RECT rect;
				GetWindowRect(hwnd, &rect);
				wnd->_true_size.x = rect.right - (rect.left < 0 ? -rect.left : rect.left);
				wnd->_true_size.y = rect.bottom - (rect.top < 0 ? -rect.top : rect.top);
			}
			return 0;
		}
		case WM_INPUT: {

			if (!wnd->_flags.is_set(wf_high_freq))
				return 0;

			UINT		dwSize = sizeof(RAWINPUT);
			static BYTE lpb[sizeof(RAWINPUT)];
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEKEYBOARD)
			{
				// Handle keyboard input
				USHORT sc = raw->data.keyboard.MakeCode;
				if (raw->data.keyboard.Flags & RI_KEY_E0)
					sc |= 0xE000;
				if (raw->data.keyboard.Flags & RI_KEY_E1)
					sc |= 0xE100;

				UINT	   key		   = MapVirtualKey(sc, MAPVK_VSC_TO_VK_EX);
				const bool is_release  = raw->data.keyboard.Flags & RI_KEY_BREAK;
				const bool is_extended = raw->data.keyboard.Flags & RI_KEY_E0;
				key					   = map_numpad_vs_extended(sc, is_extended, key);

				u8 is_repeat = 0;
				if (!is_release)
				{
					is_repeat			= s_key_down_map[key];
					s_key_down_map[key] = 1;
				}
				else
					s_key_down_map[key] = 0;

				const window_event ev = {
					.value	  = vec2i16(static_cast<i32>(sc), 0),
					.button	  = static_cast<u16>(key),
					.type	  = window_event_type::key,
					.sub_type = is_release ? window_event_sub_type::release : (is_repeat ? window_event_sub_type::repeat : window_event_sub_type::press),
					.flags	  = wef_high_freq,
				};
				wnd->add_event(ev);
			}
			else if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				USHORT mouse_flags = raw->data.mouse.usButtonFlags;
				// POINT  cursorPos;
				// GetCursorPos(&cursorPos);
				//
				// wnd->_mouse_position_abs = vec2i16(static_cast<i32>(cursorPos.x), static_cast<i32>(cursorPos.y));
				//
				// const vec2i16 relative = wnd->_mouse_position_abs - wnd->_position;
				// wnd->_mouse_position	  = vec2i16::clamp(relative, vec2i16(), vec2i16(static_cast<i16>(wnd->_size.x), static_cast<i16>(wnd->_size.y)));

				POINT screenPt;
				GetCursorPos(&screenPt);
				wnd->_mouse_position_abs = vec2i16((i32)screenPt.x, (i32)screenPt.y);

				POINT clientPt = screenPt;
				ScreenToClient(hwnd, &clientPt);
				wnd->_mouse_position = vec2i16((i16)clientPt.x, (i16)clientPt.y);

				window_event ev = {
					.value = wnd->_mouse_position,
					.type  = window_event_type::mouse,
					.flags = wef_high_freq,
				};

				bool ev_exists = false;

				if (mouse_flags & RI_MOUSE_LEFT_BUTTON_DOWN)
				{
					ev.button	= static_cast<u16>(input_code::mouse_0);
					ev.sub_type = window_event_sub_type::press;
					ev_exists	= true;
				}
				if (mouse_flags & RI_MOUSE_LEFT_BUTTON_UP)
				{
					ev.button	= static_cast<u16>(input_code::mouse_0);
					ev.sub_type = window_event_sub_type::release;
					ev_exists	= true;
				}
				if (mouse_flags & RI_MOUSE_RIGHT_BUTTON_DOWN)
				{
					ev.button	= static_cast<u16>(input_code::mouse_1);
					ev.sub_type = window_event_sub_type::press;
					ev_exists	= true;
				}
				if (mouse_flags & RI_MOUSE_RIGHT_BUTTON_UP)
				{
					ev.button	= static_cast<u16>(input_code::mouse_1);
					ev.sub_type = window_event_sub_type::release;
					ev_exists	= true;
				}
				if (mouse_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
				{
					ev.button	= static_cast<u16>(input_code::mouse_2);
					ev.sub_type = window_event_sub_type::press;
					ev_exists	= true;
				}
				if (mouse_flags & RI_MOUSE_MIDDLE_BUTTON_UP)
				{
					ev.button	= static_cast<u16>(input_code::mouse_2);
					ev.sub_type = window_event_sub_type::release;
					ev_exists	= true;
				}

				if (ev_exists)
					wnd->add_event(ev);
				else if (mouse_flags & RI_MOUSE_WHEEL)
				{
					const i16	wheelDelta = (i16)raw->data.mouse.usButtonData;
					const short wheel	   = (short)raw->data.mouse.usButtonData / (short)WHEEL_DELTA;

					const window_event mwe = {
						.value = vec2i16(0, wheelDelta),
						.type  = window_event_type::wheel,
						.flags = wef_high_freq,
					};
					wnd->add_event(mwe);
				}
				else
				{
					const i32 xPosRelative = raw->data.mouse.lLastX;
					const i32 yPosRelative = raw->data.mouse.lLastY;

					const window_event mdEvent = {
						.value = vec2i16(xPosRelative, yPosRelative),
						.type  = window_event_type::delta,
						.flags = wef_high_freq,
					};

					wnd->add_event(mdEvent);
				}
			}
			return 0;
		}
		case WM_KEYDOWN: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const WORD key_flags = HIWORD(lParam);
			const WORD scanCode	 = LOBYTE(key_flags);
			const int  extended	 = (lParam & 0x01000000) != 0;
			const bool is_repeat = (lParam & 1 << 30);
			u32		   key		 = static_cast<u32>(wParam);
			s_key_down_map[key]	 = 1;

			if (wParam == VK_SHIFT)
				key = extended == 0 ? VK_LSHIFT : VK_RSHIFT;
			else if (wParam == VK_CONTROL)
				key = extended == 0 ? VK_LCONTROL : VK_RCONTROL;

			const window_event ev = {
				.value	  = vec2i16(scanCode, 0),
				.button	  = static_cast<u16>(key),
				.type	  = window_event_type::key,
				.sub_type = is_repeat ? window_event_sub_type::repeat : window_event_sub_type::press,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_KEYUP: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const WORD key_flags = HIWORD(lParam);
			const WORD scanCode	 = LOBYTE(key_flags);
			const int  extended	 = (lParam & 0x01000000) != 0;
			u32		   key		 = static_cast<u32>(wParam);
			s_key_down_map[key]	 = 0;

			if (wParam == VK_SHIFT)
				key = extended ? VK_LSHIFT : VK_RSHIFT;
			else if (wParam == VK_CONTROL)
				key = extended ? VK_LCONTROL : VK_RCONTROL;

			const window_event ev = {

				.value = vec2i16(scanCode, 0), .button = static_cast<u16>(key), .type = window_event_type::key, .sub_type = window_event_sub_type::release};

			wnd->add_event(ev);

			return 0;
		}

		case WM_MOUSEMOVE: {
			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 xPos = GET_X_LPARAM(lParam);
			const i32 yPos = GET_Y_LPARAM(lParam);

			static vec2i16 previousPosition = vec2i16::zero;
			wnd->_mouse_position			= vec2i16(xPos, yPos);
			wnd->_mouse_position_abs		= wnd->get_position() + wnd->_mouse_position;

			const vec2i16 delta = wnd->_mouse_position - previousPosition;
			previousPosition	= wnd->_mouse_position;

			const window_event ev = {
				.value = delta,
				.type  = window_event_type::delta,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_MOUSEWHEEL: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;
			const i16		   delta = GET_WHEEL_DELTA_WPARAM(wParam) / (i16)(WHEEL_DELTA);
			const window_event mwe	 = {
				  .value = vec2i16(0, delta),
				  .type	 = window_event_type::wheel,
			  };

			wnd->add_event(mwe);

			return 0;
		}
		case WM_LBUTTONDOWN: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_0),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::press,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_LBUTTONDBLCLK: {

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_0),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::repeat,
			};
			wnd->add_event(ev);

			return 0;
		}
		case WM_RBUTTONDOWN: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_1),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::press,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_RBUTTONDBLCLK: {

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_1),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::repeat,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_MBUTTONDOWN: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_2),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::press,
			};

			wnd->add_event(ev);
			return 0;
		}
		case WM_LBUTTONUP: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_0),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::release,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_RBUTTONUP: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_1),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::release,
			};

			wnd->add_event(ev);

			return 0;
		}
		case WM_MBUTTONUP: {

			if (wnd->_flags.is_set(wf_high_freq))
				return 0;

			const i32 x = static_cast<i32>(GET_X_LPARAM(lParam));
			const i32 y = static_cast<i32>(GET_Y_LPARAM(lParam));

			const window_event ev = {

				.value	  = vec2i16(x, y),
				.button	  = static_cast<u16>(input_code::mouse_2),
				.type	  = window_event_type::mouse,
				.sub_type = window_event_sub_type::release,
			};

			wnd->add_event(ev);

			return 0;
		}
		default:
			break;
		}
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	}

	bool window::is_key_down(u16 key)
	{
		return s_key_down_map[key];
	}

	bool window::create(const char* title, u16 flags, const vec2i16& pos, const vec2u16& size)
	{
		HINSTANCE  hinst = GetModuleHandle(0);
		WNDCLASSEX wcx;
		BOOL	   exists = GetClassInfoEx(hinst, title, &wcx);

		if (!exists)
		{
			WNDCLASS wc		 = {};
			wc.lpfnWndProc	 = wnd_proc;
			wc.hInstance	 = hinst;
			wc.lpszClassName = title;
			wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
			wc.style		 = CS_DBLCLKS;

			if (!RegisterClassA(&wc))
			{
				SFG_ERR("Failed registering window class!");
				return false;
			}
		}

		const DWORD stylew	= get_style(flags);
		const DWORD exStyle = WS_EX_APPWINDOW;

		_position = pos;
		_size	  = size;
		_flags	  = flags;

		_true_size = size;
		if (flags & window_flags::wf_style_windowed)
		{
			RECT windowRect = {0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y)};
			AdjustWindowRect(&windowRect, stylew, FALSE);
			const int adjustedWidth	 = windowRect.right - windowRect.left;
			const int adjustedHeight = windowRect.bottom - windowRect.top;
			_true_size.x			 = static_cast<u32>(adjustedWidth);
			_true_size.y			 = static_cast<u32>(adjustedHeight);
		}

		HWND hwnd = CreateWindowExA(exStyle, title, title, stylew, _position.x, _position.y, _true_size.x, _true_size.y, NULL, NULL, hinst, NULL);
		if (!hwnd)
			return false;

		_platform_handle = static_cast<void*>(hinst);
		_window_handle	 = static_cast<void*>(hwnd);
		_monitor_info	 = fetch_monitor_info(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
		UI_SCALE		 = compute_ui_scale(_monitor_info);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		// Listen to raw input WM_INPUT
		RAWINPUTDEVICE Rid[2];
		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		Rid[0].usUsage	   = HID_USAGE_GENERIC_MOUSE;
		Rid[0].dwFlags	   = RIDEV_INPUTSINK;
		Rid[0].hwndTarget  = hwnd;

		Rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
		Rid[1].usUsage	   = HID_USAGE_GENERIC_KEYBOARD;
		Rid[1].dwFlags	   = RIDEV_INPUTSINK;
		Rid[1].hwndTarget  = hwnd;
		RegisterRawInputDevices(Rid, 2, sizeof(Rid[0]));

		bring_to_front();
		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);

		DragAcceptFiles(hwnd, TRUE);

		if (flags & window_flags::wf_cursor_confined_window)
			confine_cursor(cursor_confinement::window);
		set_cursor_visible(!(flags & window_flags::wf_cursor_hidden));
		return true;
	}

	void window::destroy()
	{
		if (_flags.is_set(window_flags::wf_cursor_hidden))
			set_cursor_visible(true);

		if (_window_handle)
			DestroyWindow(static_cast<HWND>(_window_handle));
	}

	void window::set_position(const vec2i16& pos)
	{
		HWND hwnd = static_cast<HWND>(_window_handle);
		SetWindowPos(hwnd, NULL, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
	}

	void window::maximize()
	{
		HWND hwnd = static_cast<HWND>(_window_handle);
		ShowWindow(hwnd, SW_MAXIMIZE);
	}
	void window::set_size(const vec2u16& full_size)
	{
		HWND hwnd = (HWND)_window_handle;

		if (IsZoomed(hwnd))
			return;

		_true_size = full_size;
		_flags.set(wf_size_dirty);

		SetWindowPos(hwnd, nullptr, 0, 0, (int)full_size.x, (int)full_size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		RECT wr{};
		GetWindowRect(hwnd, &wr);
		_true_size.x = (u32)(wr.right - wr.left);
		_true_size.y = (u32)(wr.bottom - wr.top);

		RECT cr{};
		GetClientRect(hwnd, &cr);
		_size.x = (u32)(cr.right - cr.left); // render resolution / framebuffer size
		_size.y = (u32)(cr.bottom - cr.top);
	}

	void window::set_style(window_flags flags)
	{
		HWND hwnd = static_cast<HWND>(_window_handle);
		SetWindowLongPtr(hwnd, GWL_STYLE, get_style(flags));
		SetWindowPos(hwnd, HWND_TOPMOST, _position.x, _position.y, _true_size.x, _true_size.y, SWP_SHOWWINDOW);
	}

	void window::bring_to_front()
	{
		HWND hwnd = static_cast<HWND>(_window_handle);
		OpenIcon(hwnd);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	void window::add_event(const window_event& ev)
	{
		if (!_event_callback)
			return;
		_event_callback(ev, _event_callback_user_data);
	}

	void window::confine_cursor(cursor_confinement conf)
	{
		_flags.remove(window_flags::wf_cursor_confined_window);

		HWND hwnd = static_cast<HWND>(_window_handle);

		if (conf == cursor_confinement::none)
		{
			const RECT rc_clip = {
				.left	= _prev_confinement[0],
				.top	= _prev_confinement[1],
				.right	= _prev_confinement[2],
				.bottom = _prev_confinement[3],
			};
			ClipCursor(&rc_clip);
			return;
		}

		RECT rc_old_clip;
		GetClipCursor(&rc_old_clip);
		_prev_confinement[0] = rc_old_clip.left;
		_prev_confinement[1] = rc_old_clip.right;
		_prev_confinement[2] = rc_old_clip.top;
		_prev_confinement[3] = rc_old_clip.bottom;

		if (conf == cursor_confinement::window)
		{
			RECT rc_clip;
			GetWindowRect(hwnd, &rc_clip);
			ClipCursor(&rc_clip);
			_flags.set(window_flags::wf_cursor_confined_window);
		}
		else if (conf == cursor_confinement::pointer)
		{
			POINT p;
			GetCursorPos(&p);
			RECT rect = {
				.left	= p.x,
				.top	= p.y,
				.right	= p.x,
				.bottom = p.y,
			};
			ClipCursor(&rect);
			_flags.set(window_flags::wf_cursor_confined_pointer);
		}
	}

	void window::set_cursor_visible(bool vis)
	{
		static u32 cursor_ct = 1;

		if (vis && cursor_ct != 0)
			return;

		if (!vis && cursor_ct == 0)
			return;

		if (vis)
			cursor_ct++;
		else
			cursor_ct--;

		ShowCursor(vis);
		_flags.set(window_flags::wf_cursor_hidden, !vis);
	}

	void window::set_cursor_state(cursor_state cs)
	{
		s_cursor_state = cs;
	}

	bool window::is_maximized() const
	{
		return _true_size.x == _monitor_info.work_size.x && _true_size.y == _monitor_info.work_size.y;
	}

	void window::query_all_monitors(vector<monitor_info>& out_info)
	{
		EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc, reinterpret_cast<LPARAM>(&out_info));
	}
	f32 window::get_wheel_delta()
	{
		return (f32)WHEEL_DELTA;
	}
}
