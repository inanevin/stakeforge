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

#include "simple_file_watcher.hpp"
#include "log.hpp"
#include "file_system.hpp"
#include <sfg/data/vector_util.hpp>
#include <filesystem>

namespace sfg
{
	simple_file_watcher_t::entry_t::entry_t(unique_t<std::filesystem::path> p, const char* s, u64 lm, u16 i) : path(std::move(p)), str(s), last_modified(lm), id(i)
	{
	}

	simple_file_watcher_t::entry_t::~entry_t() = default;

	void simple_file_watcher_t::add_path(const char* path, u16 optional_id)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("Can't add path to file watcher as it doesn't exist! {0}", path);
			return;
		}

		const u64 last_modified = file_system_t::get_last_modified_ticks(path);
		_paths.push_back(unique_t<entry_t>(new entry_t(make_unique<std::filesystem::path>(path), path, last_modified, optional_id)));
	}
	void simple_file_watcher_t::remove_path(const char* path)
	{
		auto it = vector_util::find_if(_paths, [path](const unique_t<entry_t>& e) -> bool { return strcmp(path, e->str.c_str()) == 0; });

		if (it != _paths.end())
			_paths.erase(it);
	}

	void simple_file_watcher_t::tick()
	{
		_ticks++;

		if (_ticks > _tick_interval)
		{
			_ticks = 0;
			watch();
		}
	}

	void simple_file_watcher_t::watch()
	{
		for (const unique_t<entry_t>& e : _paths)
		{
			const u64 ticks = file_system_t::get_last_modified_ticks(*e->path);
			if (e->last_modified != ticks)
			{
				e->last_modified = ticks;
				if (_callback)
					_callback(e->str.c_str(), e->last_modified, e->id, _callback_ud);
			}
		}
	}

	void simple_file_watcher_t::clear()
	{
		_paths.resize(0);
		_ticks = 0;
	}

} // namespace sfg
