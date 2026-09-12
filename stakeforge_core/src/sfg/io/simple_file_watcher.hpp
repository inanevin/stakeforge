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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/unique.hpp>
#include <sfg/data/vector.hpp>

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

namespace sfg
{

	typedef void (*simple_file_watcher_callback)(const char* p, u64 last_modified, u16 id, void* user_data);

	class simple_file_watcher_t
	{
	private:
		struct entry_t
		{
			unique_t<std::filesystem::path> path;
			string_t						str			  = "";
			u64								last_modified = 0;
			u16								id			  = 0;

			entry_t(unique_t<std::filesystem::path> p, const char* s, u64 lm, u16 i);
			~entry_t();
		};

	public:
		~simple_file_watcher_t()
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
		vector_t<unique_t<entry_t>>	 _paths;
		u16							 _tick_interval = 1;
		u16							 _ticks			= 0;
	};

}
