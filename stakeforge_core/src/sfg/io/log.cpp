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

#include "log.hpp"
#include "data/vector_util.hpp"
#include "data/string.hpp"

#ifdef SFG_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifdef SFG_DUMP_LOG_TRACE
#include "serialization/serialization.hpp"
#endif

#include <iostream>

namespace sfg
{

	log_t::~log_t()
	{

#ifdef SFG_DUMP_LOG_TRACE
		serialization::write_to_file(_log_trace, "sfg_log_trace.txt");
#endif
	}

	void log_t::log_impl(log_level level, const char* msg)
	{
		LOCK_GUARD(_mtx);

		log_impl(level, nullptr, msg);
	}

	void log_t::log_impl(log_level level, const char* func, const char* msg)
	{
		string_t msg_str = func == nullptr ? (string_t(msg)) : (string_t(func) + "() -> " + string_t(msg));
		msg_str += "\n";

#ifdef SFG_DUMP_LOG_TRACE
		if (level == log_level::error || level == log_level::warning)
		{
			_log_trace += msg;
			_log_trace += "\n";
		}
#endif

#ifdef SFG_PLATFORM_WINDOWS
		HANDLE hConsole;
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

		for (const listener_t& l : _listeners)
			l.f(level, msg, l.user_data);
	}

	void log_t::add_listener(unsigned int id, callback_function f, void* user_data)
	{
		_listeners.push_back({
			.user_data = user_data,
			.f		   = f,
			.id		   = id,
		});
	}

	void log_t::remove_listener(unsigned int id)
	{
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
