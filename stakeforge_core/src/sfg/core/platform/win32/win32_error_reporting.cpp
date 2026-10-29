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

#include "core/io/error_reporting.hpp"
#include "core/io/assert.hpp"
#include "core/io/file_system.hpp"
#include "core/io/log.hpp"

#include <csignal>

namespace sfg
{
#define ERROR_REPORTING_LISTENER_ID 0x45525250

	namespace
	{
		LPTOP_LEVEL_EXCEPTION_FILTER previous_exception_filter = nullptr;
		std::terminate_handler		 previous_terminate		   = nullptr;
		void (*previous_abort)(int signal)					   = nullptr;
		_invalid_parameter_handler previous_invalid_parameter  = nullptr;
		volatile LONG			   reporting_crash			   = 0;

		void report_file_failure(const char* operation, const char* path, DWORD error)
		{
			char message[2048] = {};
			_snprintf_s(message, sizeof(message), _TRUNCATE, "Could not %s the error log (Windows error %lu).\n%s", operation, error, path);
			OutputDebugStringA(message);
			MessageBoxA(nullptr, message, "Stakeforge error reporting", MB_OK | MB_ICONERROR);
		}
	}

	bool error_reporting_t::init(const char* application)
	{
		SFG_ASSERT(_file == nullptr);

		SYSTEMTIME time = {};
		GetSystemTime(&time);

		char filename[160] = {};
		_snprintf_s(filename, sizeof(filename), _TRUNCATE, "%s-%04u%02u%02u-%02u%02u%02u-%03u-%lu.log", application, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, GetCurrentProcessId());
		_filename = filename;

		if (!set_project_name(application))
			return false;

		InterlockedExchange(&reporting_crash, 0);
		previous_exception_filter = SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* exception) -> LONG {
			char reason[256] = {};
			_snprintf_s(reason, sizeof(reason), _TRUNCATE, "Unhandled Windows exception 0x%08lX at %p on thread %lu.", exception->ExceptionRecord->ExceptionCode, exception->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());

			error_reporting_t::get().report_crash(reason);
		});
		previous_terminate		  = std::set_terminate([]() {
			const std::exception_ptr exception = std::current_exception();

			if (exception)
			{
				try
				{
					std::rethrow_exception(exception);
				}
				catch (const std::exception& error)
				{
					error_reporting_t::get().report_crash(error.what());
				}
				catch (...)
				{
					error_reporting_t::get().report_crash("Unhandled non-standard C++ exception.");
				}
			}

			error_reporting_t::get().report_crash("C++ termination without an active exception.");
		});
		previous_abort			  = std::signal(SIGABRT, [](int signal) { error_reporting_t::get().report_crash("The application called abort."); });
		previous_invalid_parameter =
			_set_invalid_parameter_handler([](const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved) { error_reporting_t::get().report_crash("Invalid parameter passed to the C runtime."); });

		log_t::instance().add_listener(ERROR_REPORTING_LISTENER_ID, on_log, this);

		return true;
	}

	void error_reporting_t::uninit()
	{
		log_t::instance().remove_listener(ERROR_REPORTING_LISTENER_ID);

		LOCK_GUARD(_mutex);

		if (!write_line("Session", "Graceful shutdown."))
			report_file_failure("write", _log_path.c_str(), GetLastError());

		close_file();
		SetUnhandledExceptionFilter(previous_exception_filter);
		std::set_terminate(previous_terminate);
		std::signal(SIGABRT, previous_abort);
		_set_invalid_parameter_handler(previous_invalid_parameter);
	}

	bool error_reporting_t::set_project_name(const char* project_name)
	{
		string_t directory = file_system_t::get_user_directory();

		if (directory.empty())
		{
			SFG_ERR("could not locate the user directory for error reporting");
			return false;
		}

		file_system_t::fix_path_end_slash(directory);
		directory += "Stakeforge/";
		directory += project_name;
		directory += "/Logs/";

		if (!file_system_t::ensure_directory(directory.c_str()))
			return false;

		const string_t path = directory + _filename;

		if (path == _log_path)
			return true;

		HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (file == INVALID_HANDLE_VALUE)
		{
			SFG_ERR("could not open error log {0}: Windows error {1}", path, GetLastError());
			return false;
		}

		const LARGE_INTEGER offset = {};

		if (!SetFilePointerEx(file, offset, nullptr, FILE_END))
		{
			const DWORD error = GetLastError();

			if (!CloseHandle(file))
				report_file_failure("close", path.c_str(), GetLastError());

			SFG_ERR("could not seek to the end of error log {0}: Windows error {1}", path, error);
			return false;
		}

		LOCK_GUARD(_mutex);

		if (_file != nullptr)
			close_file();

		_file	   = file;
		_log_path  = path;
		_io_failed = false;

		return true;
	}

	void error_reporting_t::close_file()
	{
		if (!FlushFileBuffers(_file))
			report_file_failure("flush", _log_path.c_str(), GetLastError());

		if (!CloseHandle(_file))
			report_file_failure("close", _log_path.c_str(), GetLastError());

		_file = nullptr;
	}

	void error_reporting_t::on_log(log_source_e source, log_level level, const char* message, void* user_data)
	{
		if (level != log_level::warning && level != log_level::error)
			return;

		error_reporting_t& reporting = *static_cast<error_reporting_t*>(user_data);
		LOCK_GUARD(reporting._mutex);

		if (reporting._io_failed)
			return;

		if (!reporting.write_line(level == log_level::warning ? "Warn" : "Error", message))
		{
			reporting._io_failed = true;
			report_file_failure("write", reporting._log_path.c_str(), GetLastError());
		}
	}

	bool error_reporting_t::write_line(const char* level, const char* message) noexcept
	{
		SYSTEMTIME time = {};
		GetSystemTime(&time);

		char		prefix[128] = {};
		const int	prefix_size = _snprintf_s(prefix, sizeof(prefix), _TRUNCATE, "%04u-%02u-%02u %02u:%02u:%02u.%03u UTC [%lu] [%s] ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, GetCurrentThreadId(), level);
		DWORD		written		= 0;
		const DWORD message_size = static_cast<DWORD>(strlen(message));

		if (!WriteFile(_file, prefix, static_cast<DWORD>(prefix_size), &written, nullptr) || written != static_cast<DWORD>(prefix_size))
			return false;

		if (!WriteFile(_file, message, message_size, &written, nullptr) || written != message_size)
			return false;

		return WriteFile(_file, "\r\n", 2, &written, nullptr) && written == 2;
	}

	[[noreturn]] void error_reporting_t::report_crash(const char* reason) noexcept
	{
		if (InterlockedCompareExchange(&reporting_crash, 1, 0) != 0)
		{
			TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
			std::_Exit(EXIT_FAILURE);
		}

		char message[4096] = {};

		if (_mutex.try_lock())
		{
			const bool written = write_line("Crash", reason);
			const bool flushed = FlushFileBuffers(_file) != 0;
			_snprintf_s(message, sizeof(message), _TRUNCATE, "Stakeforge stopped unexpectedly.\n\n%s\n\nLog: %s%s", reason, _log_path.c_str(), written && flushed && !_io_failed ? "" : "\nThe log could not be fully written or flushed.");
			_mutex.unlock();
		}
		else
		{
			_snprintf_s(message, sizeof(message), _TRUNCATE, "Stakeforge stopped unexpectedly.\n\n%s\n\nLogging was active when the crash occurred. Completed records are in the project's Logs folder under AppData/Stakeforge.", reason);
		}

		MessageBoxA(nullptr, message, "Stakeforge crash", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
		std::_Exit(EXIT_FAILURE);
	}
}
