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

#include "platform/process.hpp"
#include "platform/window_common.hpp"
#include "io/assert.hpp"

#include "io/log.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>

namespace
{
	int enumerate_monitors(HMONITOR monitor, HDC, LPRECT, LPARAM l_param)
	{
		SFG::vector<SFG::monitor_info>* infos = reinterpret_cast<SFG::vector<SFG::monitor_info>*>(l_param);
		infos->push_back({});
		SFG::monitor_info& info = infos->back();

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
		return 1;
	}
}

namespace SFG
{
	string process::select_folder(const char* title)
	{
		string result;

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
						char buffer[MAX_PATH];
						WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, MAX_PATH, nullptr, nullptr);
						result = buffer;
						CoTaskMemFree(path);
					}
					item->Release();
				}
			}
			dialog->Release();
		}

		return result;
	}

	string process::select_file(const char* title, const char* extension)
	{
		string result;

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

	void process::select_files(const char* title, const char* extension, vector<string>& out_files)
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
									string path;
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

	string process::save_file(const char* title, const char* extension)
	{
		string result;

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

	string process::get_clipboard()
	{
		if (!OpenClipboard(nullptr))
			return string();

		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		if (!hData)
		{
			CloseClipboard();
			return string();
		}

		const wchar_t* w = static_cast<const wchar_t*>(GlobalLock(hData));
		if (!w)
		{
			CloseClipboard();
			return string();
		}

		const int needed = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
		if (needed <= 0)
		{
			GlobalUnlock(hData);
			CloseClipboard();
			return string();
		}

		std::string out;
		out.resize(static_cast<size_t>(needed - 1));
		WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), needed, nullptr, nullptr);

		GlobalUnlock(hData);
		CloseClipboard();

		return string(out.c_str());
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

	void process::init()
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		SetProcessPriorityBoost(GetCurrentProcess(), FALSE);

		// Avoid over-constraining scheduling which can cause hitches.
		// Do not pin to a single CPU and avoid realtime priority.
		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		{
			DWORD dwError = GetLastError();
			SFG_ERR("Failed setting process priority: {0}", dwError);
		}

		CoInitialize(nullptr);
	}

	void process::uninit()
	{
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

	void process::message_box(const char* msg)
	{
		MessageBox(nullptr, msg, "Huh?", MB_OK | MB_ICONERROR);
	}
	void process::get_all_monitors(vector<monitor_info>& out)
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

	u16 SFG::process::get_character_mask_from_key(u32 keycode, char ch)
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

	/*
	void process::send_pipe_data(void* data, size_t data_size)
	{
		HANDLE pipe = static_cast<HANDLE>(s_pipe_handle);
		if (pipe == INVALID_HANDLE_VALUE || pipe == nullptr)
			return;

		SFG_ASSERT(data_size < PIPE_MAX_MSG_SIZE);

		DWORD bytesWritten;
		if (!WriteFile(pipe, data, static_cast<DWORD>(data_size), &bytesWritten, NULL))
		{
			DWORD dwError = GetLastError();
			// SFG_ERR("send_pipe_data() -> Failed to write to pipe: {0}", dwError);
		}
	}*/

} // namespace SFG
