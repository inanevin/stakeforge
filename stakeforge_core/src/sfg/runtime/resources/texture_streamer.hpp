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

#include "texture.hpp"
#include <sfg/vendor/moodycamel/concurrentqueue.h>

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

		void						   enqueue(resource_entry_t& entry, resource_file_system_t& rfs);
		void						   flush_completed(resource_manager_t& resource_manager);
		static texture_stream_result_t load_result(sid_t hash, u64 source_ticks, resource_file_system_t& rfs);
		static void					   release_result(texture_stream_result_t& result);

	private:
		moodycamel::ConcurrentQueue<texture_stream_result_t> _results;
	};
}
