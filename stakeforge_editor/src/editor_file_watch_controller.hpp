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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

#include <thread>

namespace sfg
{
	struct editor_file_watch_platform_state_t;

	enum class editor_file_watch_root_e : u8
	{
		assets,
		cache,
		count,
	};

	struct editor_file_change_t
	{
		sid_t					 path_id = NULL_SID;
		editor_file_watch_root_e root	 = editor_file_watch_root_e::assets;
	};

	class editor_file_watch_controller_t final
	{
	public:
		editor_file_watch_controller_t()												 = default;
		~editor_file_watch_controller_t()												 = default;
		editor_file_watch_controller_t(const editor_file_watch_controller_t&)			 = delete;
		editor_file_watch_controller_t& operator=(const editor_file_watch_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const char* assets_path, const char* cache_path);
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline span_t<const editor_file_change_t> get_changes() const
		{
			return {.data = _changes.data(), .size = _changes.size()};
		}

	private:
		struct pending_change_t
		{
			editor_file_change_t change		   = {};
			i64					 ready_time_us = 0;
		};

		void worker_loop();

	private:
		moodycamel::ConcurrentQueue<editor_file_change_t> _raw_changes{4096};
		moodycamel::ProducerToken						  _raw_change_producer{_raw_changes};
		moodycamel::ConsumerToken						  _raw_change_consumer{_raw_changes};
		hash_map_t<sid_t, pending_change_t>				  _pending_changes;
		vector_t<editor_file_change_t>					  _changes;
		string_t										  _root_paths[static_cast<u8>(editor_file_watch_root_e::count)]			  = {};
		u64												  _root_path_hash_seeds[static_cast<u8>(editor_file_watch_root_e::count)] = {};
		std::thread										  _worker_thread;
		editor_file_watch_platform_state_t*				  _platform_state = nullptr;
		atomic_t<bool>									  _stop_requested = false;
	};
}
