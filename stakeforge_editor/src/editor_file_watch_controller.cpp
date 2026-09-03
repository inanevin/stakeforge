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

#include "editor_file_watch_controller.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <tracy/Tracy.hpp>

#ifdef SFG_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace sfg
{
#define EDITOR_FILE_CHANGE_DEBOUNCE_US		200000
#define EDITOR_FILE_CHANGE_MAX_PER_TICK		256
#define EDITOR_FILE_RAW_CHANGE_MAX_PER_TICK 4096
#define EDITOR_FILE_WATCH_BUFFER_SIZE		(64 * 1024)
#define EDITOR_FILE_WATCH_PATH_CAPACITY		2048

#ifdef SFG_PLATFORM_WINDOWS
	struct editor_file_watch_platform_state_t
	{
		struct watch_t
		{
			OVERLAPPED overlapped									= {};
			HANDLE	   handle										= INVALID_HANDLE_VALUE;
			alignas(void*) u8 buffer[EDITOR_FILE_WATCH_BUFFER_SIZE] = {};
		};

		watch_t watches[static_cast<u8>(editor_file_watch_root_e::count)] = {};
		HANDLE	completion_port											  = nullptr;
	};

	namespace
	{
		bool request_file_notifications(editor_file_watch_platform_state_t::watch_t& watch)
		{
			watch.overlapped = {};

			return ReadDirectoryChangesW(watch.handle,
										 watch.buffer,
										 EDITOR_FILE_WATCH_BUFFER_SIZE,
										 TRUE,
										 FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE,
										 nullptr,
										 &watch.overlapped,
										 nullptr) != FALSE;
		}
	}
#else
	struct editor_file_watch_platform_state_t
	{
	};
#endif

	bool editor_file_watch_controller_t::init(const char* assets_path, const char* cache_path)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(_platform_state == nullptr);

#ifndef SFG_PLATFORM_WINDOWS
		SFG_ERR("native editor file watching is not implemented on this platform");
		return false;
#else
		_root_paths[static_cast<u8>(editor_file_watch_root_e::assets)] = file_system_t::get_absolute_path(assets_path);
		_root_paths[static_cast<u8>(editor_file_watch_root_e::cache)]  = file_system_t::get_absolute_path(cache_path);

		for (u8 i = 0; i < static_cast<u8>(editor_file_watch_root_e::count); ++i)
		{
			string_t& root_path = _root_paths[i];
			file_system_t::fix_path_end_slash(root_path);

			string_t normalized_root_path = root_path;
			string_util::to_lower(normalized_root_path);
			_root_path_hash_seeds[i] = hashing_t::hash_u64(normalized_root_path.c_str(), normalized_root_path.size());
		}

		_platform_state					 = new editor_file_watch_platform_state_t();
		_platform_state->completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

		if (_platform_state->completion_port == nullptr)
		{
			SFG_ERR("failed to create file watch completion port, error {0}", GetLastError());
			delete _platform_state;
			_platform_state = nullptr;
			return false;
		}

		for (u8 i = 0; i < static_cast<u8>(editor_file_watch_root_e::count); ++i)
		{
			editor_file_watch_platform_state_t::watch_t& watch	   = _platform_state->watches[i];
			const wstring_t								 wide_path = string_util::to_wstr(_root_paths[i]);
			watch.handle = CreateFileW(wide_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

			if (watch.handle == INVALID_HANDLE_VALUE)
			{
				SFG_ERR("failed to open file watch directory {0}, error {1}", _root_paths[i], GetLastError());

				for (u8 close_i = 0; close_i < i; ++close_i)
					CloseHandle(_platform_state->watches[close_i].handle);

				CloseHandle(_platform_state->completion_port);
				delete _platform_state;
				_platform_state = nullptr;
				return false;
			}

			const HANDLE completion_port = CreateIoCompletionPort(watch.handle, _platform_state->completion_port, static_cast<ULONG_PTR>(i + 1), 0);

			if (completion_port == nullptr || !request_file_notifications(watch))
			{
				SFG_ERR("failed to start watching directory {0}, error {1}", _root_paths[i], GetLastError());

				for (u8 close_i = 0; close_i <= i; ++close_i)
					CloseHandle(_platform_state->watches[close_i].handle);

				CloseHandle(_platform_state->completion_port);
				delete _platform_state;
				_platform_state = nullptr;
				return false;
			}
		}

		_pending_changes.clear();
		_pending_changes.reserve(4096);
		_changes.resize(0);
		_changes.reserve(EDITOR_FILE_CHANGE_MAX_PER_TICK);
		_stop_requested.store(false, std::memory_order_relaxed);
		_worker_thread = std::thread(&editor_file_watch_controller_t::worker_loop, this);
		return true;
#endif
	}

	void editor_file_watch_controller_t::uninit()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(_platform_state != nullptr);

#ifdef SFG_PLATFORM_WINDOWS
		_stop_requested.store(true, std::memory_order_release);

		for (editor_file_watch_platform_state_t::watch_t& watch : _platform_state->watches)
			CancelIoEx(watch.handle, &watch.overlapped);

		PostQueuedCompletionStatus(_platform_state->completion_port, 0, 0, nullptr);
		_worker_thread.join();

		for (editor_file_watch_platform_state_t::watch_t& watch : _platform_state->watches)
			CloseHandle(watch.handle);

		CloseHandle(_platform_state->completion_port);
#endif

		delete _platform_state;
		_platform_state = nullptr;

		editor_file_change_t raw_change = {};

		while (_raw_changes.try_dequeue(_raw_change_consumer, raw_change))
		{
		}

		_pending_changes.clear();
		_changes.resize(0);
		_stop_requested.store(false, std::memory_order_relaxed);

		for (string_t& root_path : _root_paths)
			root_path.resize(0);

		for (u64& root_path_hash_seed : _root_path_hash_seeds)
			root_path_hash_seed = 0;
	}

	void editor_file_watch_controller_t::tick()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(_platform_state != nullptr);

		_changes.resize(0);

		const i64			 now_us			  = time_t::get_cpu_microseconds();
		editor_file_change_t raw_change		  = {};
		u32					 raw_change_count = 0;

		while (raw_change_count < EDITOR_FILE_RAW_CHANGE_MAX_PER_TICK && _raw_changes.try_dequeue(_raw_change_consumer, raw_change))
		{
			const sid_t pending_change_id = hashing_t::hash_u64_combine(raw_change.path_id, raw_change.root);

			pending_change_t& pending = _pending_changes.try_emplace(pending_change_id).first->second;
			pending.change			  = raw_change;
			pending.ready_time_us	  = now_us + EDITOR_FILE_CHANGE_DEBOUNCE_US;
			++raw_change_count;
		}

		for (auto it = _pending_changes.begin(); it != _pending_changes.end() && _changes.size() < EDITOR_FILE_CHANGE_MAX_PER_TICK;)
		{
			if (it->second.ready_time_us > now_us)
			{
				++it;
				continue;
			}

			const editor_file_change_t& change = it->second.change;
			_changes.push_back(change);
			it = _pending_changes.erase(it);
		}
	}

	void editor_file_watch_controller_t::worker_loop()
	{
#ifdef SFG_PLATFORM_WINDOWS
#ifdef TRACY_ENABLE
		tracy::SetThreadName("editor file watch");
#endif

		while (!_stop_requested.load(std::memory_order_acquire))
		{
			DWORD		bytes_transferred = 0;
			ULONG_PTR	completion_key	  = 0;
			OVERLAPPED* overlapped		  = nullptr;
			const BOOL	completed		  = GetQueuedCompletionStatus(_platform_state->completion_port, &bytes_transferred, &completion_key, &overlapped, INFINITE);

			if (_stop_requested.load(std::memory_order_acquire) || completion_key == 0)
				break;

			const u8									 root_index = static_cast<u8>(completion_key - 1);
			editor_file_watch_platform_state_t::watch_t& watch		= _platform_state->watches[root_index];

			if (completed == FALSE)
				SFG_ERR("file watch notification failed for {0}, error {1}; file changes were lost", _root_paths[root_index], GetLastError());
			else if (bytes_transferred == 0)
				SFG_ERR("file watch notification buffer overflowed for {0}; file changes were lost", _root_paths[root_index]);
			else
			{
				u32	 offset		  = 0;
				bool changes_lost = false;

				while (offset < bytes_transferred)
				{
					const FILE_NOTIFY_INFORMATION& notification									  = *reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(watch.buffer + offset);
					char						   relative_path[EDITOR_FILE_WATCH_PATH_CAPACITY] = {};
					const int relative_path_size = WideCharToMultiByte(CP_UTF8, 0, notification.FileName, static_cast<int>(notification.FileNameLength / sizeof(wchar_t)), relative_path, EDITOR_FILE_WATCH_PATH_CAPACITY - 1, nullptr, nullptr);

					if (relative_path_size == 0)
						changes_lost = true;
					else
					{
						relative_path[relative_path_size] = '\0';

						for (int i = 0; i < relative_path_size; ++i)
						{
							if (relative_path[i] == '\\')
								relative_path[i] = '/';
							else
								relative_path[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(relative_path[i])));
						}

						const bool enqueued = _raw_changes.enqueue(_raw_change_producer,
																   {
																	   .path_id = hashing_t::hash_u64(_root_path_hash_seeds[root_index], relative_path, static_cast<size_t>(relative_path_size)),
																	   .root	= static_cast<editor_file_watch_root_e>(root_index),
																   });

						if (!enqueued)
							changes_lost = true;
					}

					if (notification.NextEntryOffset == 0)
						break;

					offset += notification.NextEntryOffset;
				}

				if (changes_lost)
					SFG_ERR("failed to queue one or more file watch notifications for {0}; file changes were lost", _root_paths[root_index]);
			}

			if (!request_file_notifications(watch))
			{
				SFG_ERR("failed to continue watching directory {0}, error {1}; future file changes will be lost", _root_paths[root_index], GetLastError());
				break;
			}
		}
#endif
	}
}
