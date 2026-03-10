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

#include "simple_file_watcher.hpp"
#include "data/vector_util.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include <filesystem>

namespace SFG
{
	void simple_file_watcher::add_path(const char* path, u16 optional_id)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("Can't add path to file watcher as it doesn't exist! {0}", path);
			return;
		}

		const u64 last_modified = file_system::get_last_modified_ticks(path);
		_paths.push_back(new entry(new std::filesystem::path(path), path, last_modified, optional_id));
	}
	void simple_file_watcher::remove_path(const char* path)
	{
		auto it = vector_util::find_if(_paths, [path](entry* e) -> bool { return strcmp(path, e->str.c_str()) == 0; });

		if (it != _paths.end())
		{
			entry* e = *it;
			delete e->path;
			delete e;
		}
	}

	void simple_file_watcher::tick()
	{
		_ticks++;

		if (_ticks > _tick_interval)
		{
			_ticks = 0;
			watch();
		}
	}

	void simple_file_watcher::watch()
	{
		for (entry* e : _paths)
		{
			const u64 ticks = file_system::get_last_modified_ticks(*e->path);
			if (e->last_modified != ticks)
			{
				e->last_modified = ticks;
				if (_callback)
					_callback(e->str.c_str(), e->last_modified, e->id, _callback_ud);
			}
		}
	}

	void simple_file_watcher::clear()
	{
		for (const entry* p : _paths)
		{
			delete p->path;
			delete p;
		}

		_paths.resize(0);
		_ticks = 0;
	}

} // namespace SFG
