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
#include "data/string.hpp"
#include "data/vector.hpp"
#include "common/string_id.hpp"

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

namespace SFG
{

	typedef void (*simple_file_watcher_callback)(const char* p, u64 last_modified, u16 id, void* user_data);

	class simple_file_watcher
	{
	private:
		struct entry
		{
			std::filesystem::path* path			 = nullptr;
			string_t			   str			 = "";
			u64					   last_modified = 0;
			u16					   id			 = 0;

			~entry()
			{
			}
		};

	public:
		~simple_file_watcher()
		{
			clear();
		}

		void add_path(const char* path, u16 optional_id = 0);
		void remove_path(const char* path);
		void clear();
		void tick();

		inline void set_tick_interval(u16 interval)
		{
			_tick_interval = interval;
		}

		inline void set_callback(simple_file_watcher_callback cb, void* user_data)
		{
			_callback	 = cb;
			_callback_ud = user_data;
		}

		inline void reserve(int count)
		{
			_paths.reserve(count);
		}

	private:
		void watch();

	private:
		simple_file_watcher_callback _callback	  = nullptr;
		void*						 _callback_ud = nullptr;
		vector_t<entry*>			 _paths;
		u16							 _tick_interval = 1;
		u16							 _ticks			= 0;
	};

}
