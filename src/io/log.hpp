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

#ifdef SFG_DEBUG

#define SFG_ERR(...)   SFG::log::instance().log_msg_func(SFG::log_level::error, __FUNCTION__, __VA_ARGS__)
#define SFG_WARN(...)  SFG::log::instance().log_msg_func(SFG::log_level::warning, __FUNCTION__, __VA_ARGS__)
#define SFG_INFO(...)  SFG::log::instance().log_msg_func(SFG::log_level::info, __FUNCTION__, __VA_ARGS__)
#define SFG_TRACE(...) SFG::log::instance().log_msg(SFG::log_level::trace, __VA_ARGS__)
#define SFG_FATAL(...) SFG::log::instance().log_msg_func(SFG::log_level::error, __FUNCTION__, __VA_ARGS__)
#define SFG_PROG(...)  SFG::log::instance().log_msg(SFG::log_level::progress, __VA_ARGS__)

#else

#define SFG_ERR(...)   SFG::log::instance().log_msg(SFG::log_level::error, __FUNCTION__, __VA_ARGS__)
#define SFG_WARN(...)  SFG::log::instance().log_msg(SFG::log_level::warning, __FUNCTION__, __VA_ARGS__)
#define SFG_INFO(...)  SFG::log::instance().log_msg_func(SFG::log_level::info, __FUNCTION__, __VA_ARGS__)
#define SFG_TRACE(...) SFG::log::instance().log_msg(SFG::log_level::trace, __VA_ARGS__)
#define SFG_FATAL(...) SFG::log::instance().log_msg(SFG::log_level::error, __FUNCTION__, __VA_ARGS__)
#define SFG_PROG(...)  SFG::log::instance().log_msg(SFG::log_level::progress, __VA_ARGS__)

#endif

#include "data/mutex.hpp"
#include "memory/malloc_allocator_stl.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"

#include <sstream>

namespace SFG
{
	enum class log_level
	{
		info,
		error,
		trace,
		warning,
		progress,
	};

	class log
	{
	public:
		typedef void (*callback_function)(log_level lvl, const char* msg, void* user_data);

		static log& instance()
		{
			static log log;
			return log;
		}

		log()
		{
#ifdef SFG_DUMP_LOG_TRACE
			_log_trace.reserve(1024 * 10);
#endif
		}

		~log();

		// Helper to convert various types to string
		template <typename T> std::string to_str(const T& value)
		{
			std::ostringstream oss;
			oss << value;
			return oss.str();
		}

		template <typename... Args> std::string format_str(const std::string& format, const Args&... args)
		{
			std::vector<std::string> argList{to_str(args)...}; // Convert args to strings
			std::ostringstream		 result;
			size_t					 i = 0;

			while (i < format.size())
			{
				if (format[i] == '{')
				{
					size_t end = format.find('}', i);
					if (end != std::string::npos)
					{
						std::string indexStr = format.substr(i + 1, end - i - 1);
						try
						{
							size_t index = std::stoul(indexStr);
							if (index < argList.size())
							{
								result << argList[index]; // Replace with corresponding argument
							}
							else
							{
								result << "{" << indexStr << "}"; // Keep original if out of bounds
							}
						}
						catch (...)
						{
							result << "{" << indexStr << "}"; // Handle invalid indices
						}
						i = end + 1;
						continue;
					}
				}
				result << format[i++];
			}

			return result.str();
		}

		template <typename... Args> void log_msg(log_level level, const Args&... args)
		{
			log_impl(level, format_str(args...).c_str());
		}

		template <typename... Args> void log_msg_func(log_level level, const char* func, const Args&... args)
		{
			log_impl(level, func, format_str(args...).c_str());
		}

		void add_listener(unsigned int id, callback_function f, void* user_data);
		void remove_listener(unsigned int id);

	private:
		struct listener
		{
			void*			  user_data = nullptr;
			callback_function f			= nullptr;
			unsigned int	  id		= 0;
		};

	private:
		const char* get_level(log_level lvl);
		void		log_impl(log_level level, const char* msg);
		void		log_impl(log_level level, const char* func, const char* msg);

	private:
		template <typename T> using vector_malloc = std::vector<T, malloc_allocator_stl<T>>;

		mutex_t					_mtx;
		vector_malloc<listener> _listeners;
#ifdef SFG_DUMP_LOG_TRACE
		string_t _log_trace;
#endif
	};
}
