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

#include "core/io/log.hpp"
#include "core/data/vector_util.hpp"
#include "core/data/string.hpp"

#ifdef SFG_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <iostream>

namespace sfg
{
	void log_t::log_impl(log_source_e source, log_level level, const char* msg)
	{
		log_impl(source, level, nullptr, msg);
	}

	void log_t::log_impl(log_source_e source, log_level level, const char* func, const char* msg)
	{
		LOCK_GUARD(_mtx);

		string_t msg_str = func == nullptr ? (string_t(msg)) : (string_t(func) + "() -> " + string_t(msg));

		for (const listener_t& listener : _listeners)
			listener.f(source, level, msg_str.c_str(), listener.user_data);

		msg_str += "\n";

#ifdef SFG_PLATFORM_WINDOWS
		HANDLE hConsole		= nullptr;
		DWORD  console_mode = 0;
		int	   color_t		= 15;

		if (level == log_level::trace)
			color_t = 3;
		else if (level == log_level::info)
			color_t = 15;
		else if ((level == log_level::warning))
			color_t = 6;
		else if (level == log_level::error)
			color_t = 4;
		else if (level == log_level::progress)
			color_t = 8;

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hConsole != INVALID_HANDLE_VALUE && GetConsoleMode(hConsole, &console_mode))
		{
			SetConsoleTextAttribute(hConsole, color_t);
			WriteConsoleA(hConsole, msg_str.c_str(), static_cast<DWORD>(strlen(msg_str.c_str())), NULL, NULL);
		}
		else
		{
			std::cout << msg_str.c_str();
		}
#else
		std::cout << msg_str.c_str();
#endif
	}

	void log_t::add_listener(unsigned int id, callback_function f, void* user_data)
	{
		LOCK_GUARD(_mtx);

		_listeners.push_back({
			.user_data = user_data,
			.f		   = f,
			.id		   = id,
		});
	}

	void log_t::remove_listener(unsigned int id)
	{
		LOCK_GUARD(_mtx);

		std::erase_if(_listeners, [id](const listener_t& l) -> bool { return l.id == id; });
	}

	const char* log_t::get_level(log_level level)
	{
		switch (level)
		{
		case log_level::error:
			return "Error";
		case log_level::info:
			return "Info";
		case log_level::trace:
			return "Trace";
		case log_level::warning:
			return "Warn";
		case log_level::progress:
			return "Progress";
		default:
			return "";
		}
	}

}
