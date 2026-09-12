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

#include "texture.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/vendor/moodycamel/concurrentqueue.h>

#include <thread>

namespace sfg
{
	class resource_file_system_t;
	class resource_manager_t;
	struct resource_entry_t;

	struct texture_stream_result_t
	{
		sid_t			 hash							  = 0;
		u64				 source_ticks					  = 0;
		texture_buffer_t mips[texture_loader_t::MAX_MIPS] = {};
		texture_header_t header							  = {};
		bool			 success						  = false;
	};

	class texture_streamer_t final
	{
	public:
		texture_streamer_t()									 = default;
		~texture_streamer_t()									 = default;
		texture_streamer_t(const texture_streamer_t&)			 = delete;
		texture_streamer_t& operator=(const texture_streamer_t&) = delete;

		void						   init(resource_file_system_t& resource_file_system);
		void						   uninit();
		void						   enqueue(resource_entry_t& entry, size_t payload_offset);
		void						   flush_completed(resource_manager_t& resource_manager);
		static texture_stream_result_t load_result(sid_t hash, u64 source_ticks, resource_file_system_t& rfs, size_t payload_offset);
		static void					   release_result(texture_stream_result_t& result);

	private:
		enum class request_type_e : u8
		{
			load,
			stop,
		};

		struct request_t
		{
			size_t		   payload_offset = 0;
			sid_t		   hash			  = 0;
			u64			   source_ticks	  = 0;
			request_type_e type			  = request_type_e::load;
		};

		void worker_loop();

	private:
		moodycamel::ConcurrentQueue<request_t>				 _pending_requests;
		moodycamel::ConcurrentQueue<texture_stream_result_t> _results;
		std::thread											 _worker_thread;
		resource_file_system_t*								 _resource_file_system = nullptr;
		atomic_t<bool>										 _work_available	   = false;
	};
}
