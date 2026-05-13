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

#include <sfg/platform/process.hpp>
#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/platform/common_window.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <windowsx.h>
#include <hidusage.h>
#include <dwmapi.h>

namespace
{
	u8 g_key_down_map[512] = {};

	int enumerate_monitors(HMONITOR monitor, HDC, LPRECT, LPARAM l_param)
	{
		sfg::vector_t<sfg::monitor_info_t>* infos = reinterpret_cast<sfg::vector_t<sfg::monitor_info_t>*>(l_param);
		infos->push_back({});
		sfg::monitor_info_t& info = infos->back();

		MONITORINFOEX monitor_info_t;
		monitor_info_t.cbSize = sizeof(monitor_info_t);
		GetMonitorInfo(monitor, &monitor_info_t);

		UINT	dpiX, dpiY;
		HRESULT temp2	 = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
		info.size		 = {static_cast<u16>(monitor_info_t.rcMonitor.right - monitor_info_t.rcMonitor.left), static_cast<u16>(monitor_info_t.rcMonitor.bottom - monitor_info_t.rcMonitor.top)};
		info.work_size	 = {static_cast<u16>(monitor_info_t.rcWork.right - monitor_info_t.rcWork.left), static_cast<u16>(monitor_info_t.rcWork.bottom - monitor_info_t.rcWork.top)};
		info.position	 = {static_cast<i16>(monitor_info_t.rcWork.left), static_cast<i16>(monitor_info_t.rcWork.top)};
		info.device_hash = sfg::hashing_t::hash_fnv_1a64(monitor_info_t.szDevice);
		info.is_primary	 = (monitor_info_t.dwFlags & MONITORINFOF_PRIMARY) != 0;
		info.dpi		 = dpiX;
		info.dpi_scale	 = static_cast<f32>(dpiX) / 96.0f;
		return 1;
	}

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

	u32 get_style(sfg::window_style_e style)
	{
		if (style == sfg::window_style_e::app_window)
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

	sfg::monitor_info_t fetch_monitor_info(HMONITOR monitor)
	{
		sfg::monitor_info_t info;

		MONITORINFOEX monitor_info_t;
		monitor_info_t.cbSize = sizeof(monitor_info_t);
		GetMonitorInfo(monitor, &monitor_info_t);

		UINT	dpiX, dpiY;
		HRESULT temp2	 = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
		info.size		 = {static_cast<u16>(monitor_info_t.rcMonitor.right - monitor_info_t.rcMonitor.left), static_cast<u16>(monitor_info_t.rcMonitor.bottom - monitor_info_t.rcMonitor.top)};
		info.work_size	 = {static_cast<u16>(monitor_info_t.rcWork.right - monitor_info_t.rcWork.left), static_cast<u16>(monitor_info_t.rcWork.bottom - monitor_info_t.rcWork.top)};
		info.position	 = {static_cast<i16>(monitor_info_t.rcWork.left), static_cast<i16>(monitor_info_t.rcWork.top)};
		info.device_hash = sfg::hashing_t::hash_fnv_1a64(monitor_info_t.szDevice);
		info.is_primary	 = (monitor_info_t.dwFlags & MONITORINFOF_PRIMARY) != 0;
		info.dpi		 = dpiX;
		info.dpi_scale	 = static_cast<f32>(dpiX) / 96.0f;
		return info;
	}

	sfg::vec2u16_t get_window_true_size(HWND hwnd)
	{
		RECT rect{};
		GetWindowRect(hwnd, &rect);
		return {
			static_cast<u16>(rect.right - rect.left),
			static_cast<u16>(rect.bottom - rect.top),
		};
	}

	sfg::vec2u16_t get_window_client_size(HWND hwnd)
	{
		RECT rect{};
		GetClientRect(hwnd, &rect);
		return {
			static_cast<u16>(rect.right - rect.left),
			static_cast<u16>(rect.bottom - rect.top),
		};
	}

	sfg::vec2u16_t get_outer_size_for_config(const sfg::vec2u16_t& client_size, sfg::window_style_e style)
	{
		if (style != sfg::window_style_e::app_window)
			return client_size;

		RECT window_rect = {0, 0, static_cast<LONG>(client_size.x), static_cast<LONG>(client_size.y)};
		AdjustWindowRect(&window_rect, get_style(style), FALSE);
		return {
			static_cast<u16>(window_rect.right - window_rect.left),
			static_cast<u16>(window_rect.bottom - window_rect.top),
		};
	}

	void push_event(sfg::window_runtime_t& runtime, const sfg::window_event_t& ev)
	{
		if (runtime.event_callback != nullptr)
			runtime.event_callback(runtime.window_handle, ev, runtime.event_callback_user_data);
	}

	static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
	{
		sfg::window_runtime_t* runtime = reinterpret_cast<sfg::window_runtime_t*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (runtime == nullptr)
			return DefWindowProcA(hwnd, msg, w_param, l_param);

		switch (msg)
		{
		case WM_DROPFILES:
			DragFinish(reinterpret_cast<HDROP>(w_param));
			return 0;
		case WM_DPICHANGED:
		case WM_DISPLAYCHANGE:
			runtime->monitor_info = fetch_monitor_info(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
			push_event(*runtime,
					   {
						   .type = sfg::window_event_type_e::display_change,
					   });
			return 0;
		case WM_CLOSE:
			runtime->set_flag(sfg::window_runtime_flags_e::close_requested);
			return 0;
		case WM_KILLFOCUS:
			runtime->set_flag(sfg::window_runtime_flags_e::has_focus, false);
			push_event(*runtime,
					   {
						   .value = sfg::vec2i16_t(0, 0),
						   .type  = sfg::window_event_type_e::focus,
					   });
			return 0;
		case WM_SETFOCUS:
			runtime->set_flag(sfg::window_runtime_flags_e::has_focus);
			push_event(*runtime,
					   {
						   .value = sfg::vec2i16_t(1, 0),
						   .type  = sfg::window_event_type_e::focus,
					   });
			return 0;
		case WM_MOVE: {
			RECT rect{};
			GetWindowRect(hwnd, &rect);
			runtime->pos = sfg::vec2i16_t(static_cast<i16>(rect.left), static_cast<i16>(rect.top));

			const sfg::monitor_info_t mi = fetch_monitor_info(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
			if (runtime->monitor_info.device_hash != mi.device_hash)
				runtime->monitor_info = mi;

			push_event(*runtime,
					   {
						   .value = runtime->pos,
						   .type  = sfg::window_event_type_e::repos,
					   });
			return 0;
		}
		case WM_SIZE: {
			const sfg::vec2u16_t size = {
				static_cast<u16>(LOWORD(l_param)),
				static_cast<u16>(HIWORD(l_param)),
			};
			const bool minimized		 = w_param == SIZE_MINIMIZED;
			const bool minimized_changed = runtime->has_flag(sfg::window_runtime_flags_e::minimized) != minimized;

			runtime->set_flag(sfg::window_runtime_flags_e::minimized, minimized);

			if (runtime->size.x == size.x && runtime->size.y == size.y && !minimized_changed)
				return 0;

			runtime->size	   = size;
			runtime->true_size = get_window_true_size(hwnd);
			push_event(*runtime,
					   {
						   .value = sfg::vec2i16_t(static_cast<i16>(size.x), static_cast<i16>(size.y)),
						   .type  = sfg::window_event_type_e::resize,
					   });
			return 0;
		}
		case WM_INPUT: {
			if (!runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			UINT raw_size					  = sizeof(RAWINPUT);
			BYTE raw_buffer[sizeof(RAWINPUT)] = {};
			GetRawInputData(reinterpret_cast<HRAWINPUT>(l_param), RID_INPUT, raw_buffer, &raw_size, sizeof(RAWINPUTHEADER));
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(raw_buffer);

			if (raw->header.dwType == RIM_TYPEKEYBOARD)
			{
				USHORT sc = raw->data.keyboard.MakeCode;
				if ((raw->data.keyboard.Flags & RI_KEY_E0) != 0)
					sc |= 0xE000;
				if ((raw->data.keyboard.Flags & RI_KEY_E1) != 0)
					sc |= 0xE100;

				const bool is_release  = (raw->data.keyboard.Flags & RI_KEY_BREAK) != 0;
				const bool is_extended = (raw->data.keyboard.Flags & RI_KEY_E0) != 0;
				UINT	   key		   = MapVirtualKey(sc, MAPVK_VSC_TO_VK_EX);
				key					   = map_numpad_vs_extended(sc, is_extended, key);

				u8 is_repeat = 0;
				if (!is_release)
				{
					is_repeat			= g_key_down_map[key];
					g_key_down_map[key] = 1;
				}
				else
					g_key_down_map[key] = 0;

				push_event(*runtime,
						   {
							   .value	 = sfg::vec2i16_t(static_cast<i16>(sc), 0),
							   .button	 = static_cast<u16>(key),
							   .type	 = sfg::window_event_type_e::key,
							   .sub_type = is_release ? sfg::window_event_sub_type_e::release : (is_repeat ? sfg::window_event_sub_type_e::repeat : sfg::window_event_sub_type_e::press),
							   .flags	 = sfg::wef_high_freq,
						   });
				return 0;
			}

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				POINT screen_pt{};
				GetCursorPos(&screen_pt);
				runtime->mouse_position_abs = sfg::vec2i16_t(static_cast<i16>(screen_pt.x), static_cast<i16>(screen_pt.y));

				POINT client_pt = screen_pt;
				ScreenToClient(hwnd, &client_pt);
				runtime->mouse_position = sfg::vec2i16_t(static_cast<i16>(client_pt.x), static_cast<i16>(client_pt.y));

				sfg::window_event_t ev = {
					.value = runtime->mouse_position,
					.type  = sfg::window_event_type_e::mouse,
					.flags = sfg::wef_high_freq,
				};

				bool		 emit_mouse	 = false;
				const USHORT mouse_flags = raw->data.mouse.usButtonFlags;
				if ((mouse_flags & RI_MOUSE_LEFT_BUTTON_DOWN) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_0);
					ev.sub_type = sfg::window_event_sub_type_e::press;
					emit_mouse	= true;
				}
				if ((mouse_flags & RI_MOUSE_LEFT_BUTTON_UP) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_0);
					ev.sub_type = sfg::window_event_sub_type_e::release;
					emit_mouse	= true;
				}
				if ((mouse_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_1);
					ev.sub_type = sfg::window_event_sub_type_e::press;
					emit_mouse	= true;
				}
				if ((mouse_flags & RI_MOUSE_RIGHT_BUTTON_UP) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_1);
					ev.sub_type = sfg::window_event_sub_type_e::release;
					emit_mouse	= true;
				}
				if ((mouse_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_2);
					ev.sub_type = sfg::window_event_sub_type_e::press;
					emit_mouse	= true;
				}
				if ((mouse_flags & RI_MOUSE_MIDDLE_BUTTON_UP) != 0)
				{
					ev.button	= static_cast<u16>(sfg::input_code::mouse_2);
					ev.sub_type = sfg::window_event_sub_type_e::release;
					emit_mouse	= true;
				}

				if (emit_mouse)
				{
					push_event(*runtime, ev);
					return 0;
				}

				if ((mouse_flags & RI_MOUSE_WHEEL) != 0)
				{
					push_event(*runtime,
							   {
								   .value = sfg::vec2i16_t(0, static_cast<i16>(raw->data.mouse.usButtonData)),
								   .type  = sfg::window_event_type_e::wheel,
								   .flags = sfg::wef_high_freq,
							   });
					return 0;
				}

				push_event(*runtime,
						   {
							   .value = sfg::vec2i16_t(static_cast<i16>(raw->data.mouse.lLastX), static_cast<i16>(raw->data.mouse.lLastY)),
							   .type  = sfg::window_event_type_e::delta,
							   .flags = sfg::wef_high_freq,
						   });
				return 0;
			}
			return 0;
		}
		case WM_KEYDOWN: {
			if (runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			const WORD scan_code = LOBYTE(HIWORD(l_param));
			const bool extended	 = (l_param & 0x01000000) != 0;
			const bool is_repeat = (l_param & (1 << 30)) != 0;
			u32		   key		 = static_cast<u32>(w_param);
			g_key_down_map[key]	 = 1;

			if (w_param == VK_SHIFT)
				key = extended ? VK_RSHIFT : VK_LSHIFT;
			else if (w_param == VK_CONTROL)
				key = extended ? VK_RCONTROL : VK_LCONTROL;

			push_event(*runtime,
					   {
						   .value	 = sfg::vec2i16_t(static_cast<i16>(scan_code), 0),
						   .button	 = static_cast<u16>(key),
						   .type	 = sfg::window_event_type_e::key,
						   .sub_type = is_repeat ? sfg::window_event_sub_type_e::repeat : sfg::window_event_sub_type_e::press,
					   });
			return 0;
		}
		case WM_KEYUP: {
			if (runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			const WORD scan_code = LOBYTE(HIWORD(l_param));
			const bool extended	 = (l_param & 0x01000000) != 0;
			u32		   key		 = static_cast<u32>(w_param);
			g_key_down_map[key]	 = 0;

			if (w_param == VK_SHIFT)
				key = extended ? VK_RSHIFT : VK_LSHIFT;
			else if (w_param == VK_CONTROL)
				key = extended ? VK_RCONTROL : VK_LCONTROL;

			push_event(*runtime,
					   {
						   .value	 = sfg::vec2i16_t(static_cast<i16>(scan_code), 0),
						   .button	 = static_cast<u16>(key),
						   .type	 = sfg::window_event_type_e::key,
						   .sub_type = sfg::window_event_sub_type_e::release,
					   });
			return 0;
		}
		case WM_MOUSEMOVE: {
			if (runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			const sfg::vec2i16_t previous = runtime->mouse_position;
			runtime->mouse_position		  = sfg::vec2i16_t(static_cast<i16>(GET_X_LPARAM(l_param)), static_cast<i16>(GET_Y_LPARAM(l_param)));
			runtime->mouse_position_abs	  = runtime->pos + runtime->mouse_position;

			push_event(*runtime,
					   {
						   .value = runtime->mouse_position - previous,
						   .type  = sfg::window_event_type_e::delta,
					   });
			return 0;
		}
		case WM_MOUSEWHEEL:
			if (runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			push_event(*runtime,
					   {
						   .value = sfg::vec2i16_t(0, static_cast<i16>(GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA)),
						   .type  = sfg::window_event_type_e::wheel,
					   });
			return 0;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP: {
			if (runtime->has_flag(sfg::window_runtime_flags_e::high_frequency_input))
				return 0;

			u16 button = static_cast<u16>(sfg::input_code::mouse_0);
			if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK || msg == WM_RBUTTONUP)
				button = static_cast<u16>(sfg::input_code::mouse_1);
			else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP)
				button = static_cast<u16>(sfg::input_code::mouse_2);

			sfg::window_event_sub_type_e sub_type = sfg::window_event_sub_type_e::press;
			if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP || msg == WM_MBUTTONUP)
				sub_type = sfg::window_event_sub_type_e::release;
			else if (msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDBLCLK)
				sub_type = sfg::window_event_sub_type_e::repeat;

			push_event(*runtime,
					   {
						   .value	 = sfg::vec2i16_t(static_cast<i16>(GET_X_LPARAM(l_param)), static_cast<i16>(GET_Y_LPARAM(l_param))),
						   .button	 = button,
						   .type	 = sfg::window_event_type_e::mouse,
						   .sub_type = sub_type,
					   });
			return 0;
		}
		default:
			break;
		}

		return DefWindowProcA(hwnd, msg, w_param, l_param);
	}

}

namespace sfg
{
	int process::s_prev_clip[4] = {};

	void process::init()
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		SetProcessPriorityBoost(GetCurrentProcess(), FALSE);

		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		{
			DWORD dwError = GetLastError();
			SFG_ERR("failed setting process priority: {0}", dwError);
		}

		const HRESULT res = CoInitialize(nullptr);

		RECT old_clip{};
		GetClipCursor(&old_clip);
		s_prev_clip[0] = old_clip.left;
		s_prev_clip[1] = old_clip.top;
		s_prev_clip[2] = old_clip.right;
		s_prev_clip[3] = old_clip.bottom;
	}

	void process::uninit()
	{
		CoUninitialize();
	}

	void process::pump_os_messages()
	{
		MSG msg	   = {0};
		msg.wParam = 0;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void process::open_url(const char* url)
	{
		ShellExecute(0, "open", url, NULL, NULL, SW_SHOWNORMAL);
	}

	bool process::open_directory(const char* dir)
	{
		if (!dir || !*dir)
			return false;

		// Opens the folder in Explorer
		HINSTANCE r = ShellExecuteA(nullptr, "open", dir, nullptr, nullptr, SW_SHOWNORMAL);

		return (reinterpret_cast<INT_PTR>(r) > 32);
	}

	void process::message_box(const char* title, const char* msg)
	{
		MessageBox(nullptr, msg, title, MB_OK | MB_ICONERROR);
	}

	void process::select_files(const char* title, const char* extension, vector_t<string_t>& out_files)
	{
		out_files.clear();

		IFileOpenDialog* dialog = nullptr;
		HRESULT			 hr		= CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
		if (FAILED(hr))
			return;

		// Title (UTF-8 -> UTF-16)
		if (title && title[0] != '\0')
		{
			int lenW = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
			if (lenW > 0)
			{
				std::wstring titleW;
				titleW.resize((size_t)lenW - 1);
				MultiByteToWideChar(CP_UTF8, 0, title, -1, titleW.data(), lenW);
				dialog->SetTitle(titleW.c_str());
			}
		}

		// Options
		DWORD options = 0;
		if (SUCCEEDED(dialog->GetOptions(&options)))
		{
			dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_ALLOWMULTISELECT);
		}

		// Extension filter (expects e.g. "png" or ".png")
		COMDLG_FILTERSPEC filter[1] = {};
		std::wstring	  extW;
		std::wstring	  patternW;
		std::wstring	  labelW;

		if (extension && extension[0] != '\0')
		{
			const char* ext = extension;
			if (ext[0] == '.')
				++ext;

			int extLenW = MultiByteToWideChar(CP_UTF8, 0, ext, -1, nullptr, 0);
			if (extLenW > 0)
			{
				extW.resize((size_t)extLenW - 1);
				MultiByteToWideChar(CP_UTF8, 0, ext, -1, extW.data(), extLenW);

				labelW	 = L"*." + extW;
				patternW = L"*." + extW;

				filter[0].pszName = labelW.c_str();
				filter[0].pszSpec = patternW.c_str();
				dialog->SetFileTypes(1, filter);
				dialog->SetFileTypeIndex(1);
				dialog->SetDefaultExtension(extW.c_str());
			}
		}

		hr = dialog->Show(nullptr);
		if (SUCCEEDED(hr))
		{
			IShellItemArray* items = nullptr;
			if (SUCCEEDED(dialog->GetResults(&items)) && items)
			{
				DWORD count = 0;
				if (SUCCEEDED(items->GetCount(&count)))
				{
					out_files.reserve((size_t)count);

					for (DWORD i = 0; i < count; ++i)
					{
						IShellItem* item = nullptr;
						if (SUCCEEDED(items->GetItemAt(i, &item)) && item)
						{
							PWSTR pathW = nullptr;
							if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &pathW)) && pathW)
							{
								// UTF-16 -> UTF-8
								int needed = WideCharToMultiByte(CP_UTF8, 0, pathW, -1, nullptr, 0, nullptr, nullptr);
								if (needed > 0)
								{
									string_t path;
									path.resize((size_t)needed - 1);
									WideCharToMultiByte(CP_UTF8, 0, pathW, -1, path.data(), needed, nullptr, nullptr);
									out_files.push_back(std::move(path));
								}
								CoTaskMemFree(pathW);
							}
							item->Release();
						}
					}
				}
				items->Release();
			}
		}

		dialog->Release();
	}

	string_t process::select_folder(const char* title)
	{
		string_t result;

		IFileDialog* dialog = nullptr;
		HRESULT		 hr		= CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));

		if (SUCCEEDED(hr))
		{
			DWORD options;
			if (SUCCEEDED(dialog->GetOptions(&options)))
			{
				dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			}

			hr = dialog->Show(nullptr);
			if (SUCCEEDED(hr))
			{
				IShellItem* item = nullptr;
				if (SUCCEEDED(dialog->GetResult(&item)))
				{
					PWSTR path = nullptr;
					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
					{
						char buffer_t[MAX_PATH];
						WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer_t, MAX_PATH, nullptr, nullptr);
						result = buffer_t;
						CoTaskMemFree(path);
					}
					item->Release();
				}
			}
			dialog->Release();
		}

		return result;
	}

	string_t process::select_file(const char* title, const char* extension)
	{
		string_t result;

		IFileDialog* dialog = nullptr;
		HRESULT		 hr		= CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
		if (FAILED(hr))
			return result;

		// Title
		if (title && title[0] != '\0')
			dialog->SetTitle(reinterpret_cast<LPCWSTR>(std::wstring(title, title + strlen(title)).c_str())); // see note below

		// Options
		DWORD options = 0;
		if (SUCCEEDED(dialog->GetOptions(&options)))
		{
			// FOS_FORCEFILESYSTEM ensures we get a real filesystem path.
			dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
		}

		// Extension filter (expects e.g. "png" or ".png")
		COMDLG_FILTERSPEC filter[1] = {};
		std::wstring	  extW;
		std::wstring	  patternW;
		std::wstring	  labelW;

		if (extension && extension[0] != '\0')
		{
			// normalize to "png" (no dot)
			const char* ext = extension;
			if (ext[0] == '.')
				++ext;

			// UTF-8 -> UTF-16
			int extLenW = MultiByteToWideChar(CP_UTF8, 0, ext, -1, nullptr, 0);
			if (extLenW > 0)
			{
				extW.resize((size_t)extLenW - 1);
				MultiByteToWideChar(CP_UTF8, 0, ext, -1, extW.data(), extLenW);

				labelW	 = L"*." + extW;
				patternW = L"*." + extW;

				filter[0].pszName = labelW.c_str();	  // shown in UI
				filter[0].pszSpec = patternW.c_str(); // actual filter
				dialog->SetFileTypes(1, filter);
				dialog->SetFileTypeIndex(1);
				dialog->SetDefaultExtension(extW.c_str());
			}
		}

		hr = dialog->Show(nullptr);
		if (SUCCEEDED(hr))
		{
			IShellItem* item = nullptr;
			if (SUCCEEDED(dialog->GetResult(&item)))
			{
				PWSTR path = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
				{
					// UTF-16 -> UTF-8
					int needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
					if (needed > 0)
					{
						result.resize((size_t)needed - 1);
						WideCharToMultiByte(CP_UTF8, 0, path, -1, result.data(), needed, nullptr, nullptr);
					}
					CoTaskMemFree(path);
				}
				item->Release();
			}
		}

		dialog->Release();
		return result;
	}

	string_t process::save_file(const char* title, const char* extension)
	{
		string_t result;

		IFileDialog* dialog = nullptr;
		HRESULT		 hr		= CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
		if (FAILED(hr) || dialog == nullptr)
			return result;

		// Title (UTF-8 -> UTF-16)
		if (title && title[0] != '\0')
		{
			int wlen = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
			if (wlen > 0)
			{
				std::wstring titleW;
				titleW.resize((size_t)wlen - 1);
				MultiByteToWideChar(CP_UTF8, 0, title, -1, titleW.data(), wlen);
				dialog->SetTitle(titleW.c_str());
			}
		}

		// Options
		DWORD options = 0;
		if (SUCCEEDED(dialog->GetOptions(&options)))
		{
			// FOS_FORCEFILESYSTEM ensures SIGDN_FILESYSPATH returns a real filesystem path.
			// FOS_PATHMUSTEXIST prevents choosing a non-existent folder.
			dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
		}

		// Extension filter (expects e.g. "png" or ".png")
		COMDLG_FILTERSPEC filter[1] = {};
		std::wstring	  extW;
		std::wstring	  patternW;
		std::wstring	  labelW;

		if (extension && extension[0] != '\0')
		{
			// normalize to "png" (no dot)
			const char* ext = extension;
			if (ext[0] == '.')
				++ext;

			// UTF-8 -> UTF-16
			int extLenW = MultiByteToWideChar(CP_UTF8, 0, ext, -1, nullptr, 0);
			if (extLenW > 0)
			{
				extW.resize((size_t)extLenW - 1);
				MultiByteToWideChar(CP_UTF8, 0, ext, -1, extW.data(), extLenW);

				labelW	 = L"*." + extW;
				patternW = L"*." + extW;

				filter[0].pszName = labelW.c_str();	  // shown in UI
				filter[0].pszSpec = patternW.c_str(); // actual filter
				dialog->SetFileTypes(1, filter);
				dialog->SetFileTypeIndex(1);
				dialog->SetDefaultExtension(extW.c_str());
			}
		}

		hr = dialog->Show(nullptr);
		if (SUCCEEDED(hr))
		{
			IShellItem* item = nullptr;
			if (SUCCEEDED(dialog->GetResult(&item)) && item != nullptr)
			{
				PWSTR pathW = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &pathW)) && pathW != nullptr)
				{
					// UTF-16 -> UTF-8
					int needed = WideCharToMultiByte(CP_UTF8, 0, pathW, -1, nullptr, 0, nullptr, nullptr);
					if (needed > 0)
					{
						result.resize((size_t)needed - 1);
						WideCharToMultiByte(CP_UTF8, 0, pathW, -1, result.data(), needed, nullptr, nullptr);
					}
					CoTaskMemFree(pathW);
				}
				item->Release();
			}
		}

		dialog->Release();
		return result;
	}

	string_t process::get_clipboard()
	{
		if (!OpenClipboard(nullptr))
			return string_t();

		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (!hData)
		{
			CloseClipboard();
			return string_t();
		}

		const wchar_t* w = static_cast<const wchar_t*>(GlobalLock(hData));
		if (!w)
		{
			CloseClipboard();
			return string_t();
		}

		const int needed = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
		if (needed <= 0)
		{
			GlobalUnlock(hData);
			CloseClipboard();
			return string_t();
		}

		std::string out;
		out.resize(static_cast<size_t>(needed - 1));
		WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), needed, nullptr, nullptr);

		GlobalUnlock(hData);
		CloseClipboard();

		return string_t(out.c_str());
	}

	void process::push_clipboard(const char* cp)
	{
		if (!cp)
			cp = "";

		const int wNeeded = MultiByteToWideChar(CP_UTF8, 0, cp, -1, nullptr, 0);
		if (wNeeded <= 0)
			return;

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wNeeded) * sizeof(wchar_t));
		if (!hMem)
			return;

		wchar_t* wBuf = static_cast<wchar_t*>(GlobalLock(hMem));
		if (!wBuf)
		{
			GlobalFree(hMem);
			return;
		}

		MultiByteToWideChar(CP_UTF8, 0, cp, -1, wBuf, wNeeded);
		GlobalUnlock(hMem);

		if (!OpenClipboard(nullptr))
		{
			GlobalFree(hMem);
			return;
		}

		if (!EmptyClipboard())
		{
			CloseClipboard();
			GlobalFree(hMem);
			return;
		}

		if (!SetClipboardData(CF_UNICODETEXT, hMem))
		{
			CloseClipboard();
			GlobalFree(hMem);
			return;
		}

		CloseClipboard();
	}

	void process::get_all_monitors(vector_t<monitor_info_t>& out)
	{
		EnumDisplayMonitors(NULL, NULL, enumerate_monitors, (LPARAM)&out);
	}

	char process::get_character_from_key(u32 vk)
	{
		BYTE ks[256];
		if (!GetKeyboardState(ks))
			return 0;

		auto patch = [&](int vkey) {
			if (GetAsyncKeyState(vkey) & 0x8000)
				ks[vkey] |= 0x80;
			else
				ks[vkey] &= ~0x80;
		};

		patch(VK_SHIFT);
		patch(VK_CONTROL);
		patch(VK_MENU); // Alt

		HKL layout = GetKeyboardLayout(0);

		UINT sc = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, layout);

		wchar_t buf[8] = {};
		int		rc	   = ToUnicodeEx(vk, sc, ks, buf, (int)std::size(buf), 0, layout);

		if (rc == -1)
		{
			// Dead key: flush state so next call isn't affected
			wchar_t dummy[8];
			ToUnicodeEx(vk, sc, ks, dummy, (int)std::size(dummy), 0, layout);
			return 0;
		}

		if (rc > 0)
			return buf[0];

		return 0;
	}

	u16 process::get_character_mask_from_key(u32 keycode, char ch)
	{
		u16 mask = 0;

		if (ch == L' ')
			mask |= whitespace;
		else
		{
			if (IsCharAlphaNumericA(ch))
			{
				if (keycode >= '0' && keycode <= '9')
					mask |= number;
				else if ((keycode >= '0' && keycode <= '9') || (keycode >= VK_NUMPAD0 && keycode <= VK_NUMPAD9))
				{
					mask |= number;
				}
				else
					mask |= letter;
			}
			else if (iswctype(ch, _PUNCT))
			{
				mask |= symbol;

				if (ch == '-' || ch == '+' || ch == '*' || ch == '/')
					mask |= op;

				if (ch == L'-' || ch == L'+')
					mask |= sign;
			}
			else
				mask |= control;
		}

		if (ch == L'.' || ch == L',')
			mask |= separator;

		if (mask & (letter | number | whitespace | separator | symbol))
			mask |= printable;

		return mask;
	}

	bool process::create_window(const char* title, const vec2i16_t& pos, const vec2u16_t& size, window_style_e window_style, window_runtime_t& runtime)
	{
		HINSTANCE  instance = GetModuleHandle(nullptr);
		WNDCLASSEX class_info{};
		const BOOL exists = GetClassInfoExA(instance, title, &class_info);

		if (!exists)
		{
			WNDCLASS wc{};
			wc.lpfnWndProc	 = wnd_proc;
			wc.hInstance	 = instance;
			wc.lpszClassName = title;
			wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
			wc.style		 = CS_DBLCLKS;

			if (!RegisterClassA(&wc))
			{
				SFG_ERR("Failed registering window class!");
				return false;
			}
		}

		const DWORD stylew	   = get_style(window_style);
		const DWORD ex_style   = WS_EX_APPWINDOW;
		const auto	outer_size = get_outer_size_for_config(size, window_style);

		HWND hwnd = CreateWindowExA(ex_style, title, title, stylew, pos.x, pos.y, outer_size.x, outer_size.y, nullptr, nullptr, instance, nullptr);
		if (hwnd == nullptr)
			return false;

		runtime.monitor_info	= fetch_monitor_info(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
		runtime.pos				= pos;
		runtime.size			= size;
		runtime.style			= window_style;
		runtime.window_handle	= hwnd;
		runtime.platform_handle = instance;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&runtime));

		RAWINPUTDEVICE raw_devices[2] = {};
		raw_devices[0].usUsagePage	  = HID_USAGE_PAGE_GENERIC;
		raw_devices[0].usUsage		  = HID_USAGE_GENERIC_MOUSE;
		raw_devices[0].dwFlags		  = RIDEV_INPUTSINK;
		raw_devices[0].hwndTarget	  = hwnd;
		raw_devices[1].usUsagePage	  = HID_USAGE_PAGE_GENERIC;
		raw_devices[1].usUsage		  = HID_USAGE_GENERIC_KEYBOARD;
		raw_devices[1].dwFlags		  = RIDEV_INPUTSINK;
		raw_devices[1].hwndTarget	  = hwnd;
		RegisterRawInputDevices(raw_devices, 2, sizeof(raw_devices[0]));
		OpenIcon(hwnd);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);
		DragAcceptFiles(hwnd, TRUE);

		return true;
	}

	void process::destroy_window(void* window_handle)
	{
		HWND hwnd = static_cast<HWND>(window_handle);
		SFG_ASSERT(hwnd != nullptr);
		DestroyWindow(hwnd);
	}

	void process::set_window_runtime(void* window_handle, window_runtime_t& runtime)
	{
		HWND hwnd = static_cast<HWND>(window_handle);
		SFG_ASSERT(hwnd != nullptr);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&runtime));
	}

	void process::set_window_position(void* window, const sfg::vec2i16_t& pos)
	{
		HWND hwnd = static_cast<HWND>(window);
		SetWindowPos(hwnd, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void process::set_window_size(void* window, const sfg::vec2u16_t& size, window_style_e style)
	{
		HWND hwnd = static_cast<HWND>(window);

		if (IsZoomed(hwnd))
			return;

		const sfg::vec2u16_t outer_size = get_outer_size_for_config(size, style);
		SetWindowPos(hwnd, nullptr, 0, 0, outer_size.x, outer_size.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void process::set_window_style(void* window, const vec2u16_t& size, window_style_e style)
	{
		HWND		hwnd	   = static_cast<HWND>(window);
		const DWORD stylew	   = get_style(style);
		const auto	outer_size = get_outer_size_for_config(size, style);
		SetWindowLongPtr(hwnd, GWL_STYLE, static_cast<LONG_PTR>(stylew));
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, outer_size.x, outer_size.y, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	void process::minimize_window(void* window)
	{
		HWND hwnd = static_cast<HWND>(window);
		SFG_ASSERT(hwnd != nullptr);
		ShowWindow(hwnd, SW_MINIMIZE);
	}

	void process::toggle_maximize_window(void* window)
	{
		HWND hwnd = static_cast<HWND>(window);
		SFG_ASSERT(hwnd != nullptr);
		ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
	}

	void process::set_cursor_confinement(void* window_handle, window_cursor_confinement_e conf)
	{
		HWND hwnd = static_cast<HWND>(window_handle);
		SFG_ASSERT(hwnd != nullptr);

		if (conf == sfg::window_cursor_confinement_e::none)
		{
			const RECT rect = {
				s_prev_clip[0],
				s_prev_clip[1],
				s_prev_clip[2],
				s_prev_clip[3],
			};
			ClipCursor(&rect);
			return;
		}

		RECT old_clip{};
		GetClipCursor(&old_clip);
		s_prev_clip[0] = old_clip.left;
		s_prev_clip[1] = old_clip.top;
		s_prev_clip[2] = old_clip.right;
		s_prev_clip[3] = old_clip.bottom;

		if (conf == sfg::window_cursor_confinement_e::window)
		{
			RECT clip_rect{};
			GetWindowRect(hwnd, &clip_rect);
			ClipCursor(&clip_rect);
			return;
		}

		POINT cursor_pos{};
		GetCursorPos(&cursor_pos);
		const RECT clip_rect = {
			cursor_pos.x,
			cursor_pos.y,
			cursor_pos.x,
			cursor_pos.y,
		};
		ClipCursor(&clip_rect);
	}

	void process::set_cursor_state(window_cursor_state_e state)
	{
		LPCTSTR cursor_id = IDC_ARROW;

		switch (state)
		{
		case sfg::window_cursor_state_e::arrow:
			cursor_id = IDC_ARROW;
			break;
		case sfg::window_cursor_state_e::hand:
			cursor_id = IDC_HAND;
			break;
		case sfg::window_cursor_state_e::resize_hr:
			cursor_id = IDC_SIZEWE;
			break;
		case sfg::window_cursor_state_e::resize_vt:
			cursor_id = IDC_SIZENS;
			break;
		case sfg::window_cursor_state_e::resize_nwse:
			cursor_id = IDC_SIZENWSE;
			break;
		case sfg::window_cursor_state_e::resize_nesw:
			cursor_id = IDC_SIZENESW;
			break;
		case sfg::window_cursor_state_e::caret:
			cursor_id = IDC_IBEAM;
			break;
		}

		::SetCursor(::LoadCursor(nullptr, cursor_id));
	}

	void process::set_cursor_visible(bool visible)
	{
		static i32 cursor_count = 0;

		if (visible)
		{
			while (cursor_count < 0)
				cursor_count = ShowCursor(TRUE);
			return;
		}

		while (cursor_count >= 0)
			cursor_count = ShowCursor(FALSE);
	}
}
