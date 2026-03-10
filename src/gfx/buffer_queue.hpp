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
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/barrier_description.hpp"

namespace SFG
{
	class buffer;
	class buffer_cpu_gpu;

	class buffer_queue
	{
	public:
		struct buffer_request
		{
			buffer_cpu_gpu* buffer	 = nullptr;
			resource_state	to_state = resource_state::resource_state_ps_resource;
		};

		struct update_request
		{
			buffer*		   buffer	 = nullptr;
			u8*			   data		 = nullptr;
			size_t		   data_size = 0;
			resource_state to_state	 = resource_state::resource_state_ps_resource;
		};

		struct per_frame_data
		{
			vector<update_request> buffered_requests = {};
		};

		void init();
		void uninit();
		void add_request(const buffer_request& req);
		void add_request(const update_request& req, u8 frame_index);
		void flush_all(gfx_id cmd, u8 frame_index, vector<barrier>& out_barriers);
		bool empty(u8 frame_index) const;

	private:
		per_frame_data		   _pfd[BACK_BUFFER_COUNT];
		vector<buffer_request> _requests = {};
	};
}
