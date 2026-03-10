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

#include "buffer_queue.hpp"
#include "gfx/buffer.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "memory/memory.hpp"

namespace SFG
{

	void buffer_queue::init()
	{
		_requests.reserve(256);
		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
			_pfd[i].buffered_requests.reserve(256);
	}

	void buffer_queue::uninit()
	{
	}

	void buffer_queue::add_request(const buffer_request& req)
	{
		_requests.push_back(req);
	}

	void buffer_queue::add_request(const update_request& req, u8 frame_index)
	{
		_pfd[frame_index].buffered_requests.push_back(req);
	}

	void buffer_queue::flush_all(gfx_id cmd_list, u8 frame_index, vector<barrier>& out_barriers)
	{
		gfx_backend*	backend = gfx_backend::get();
		per_frame_data& pfd		= _pfd[frame_index];

		// perform copies.
		for (const buffer_request& buf : _requests)
		{
			buf.buffer->copy(cmd_list);

			out_barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = buf.to_state,
				.resource	 = buf.buffer->get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		for (update_request& buf : pfd.buffered_requests)
		{
			buf.buffer->buffer_data(0, buf.data, buf.data_size);
			buf.buffer->copy(cmd_list);
			SFG_FREE(buf.data);

			out_barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = buf.to_state,
				.resource	 = buf.buffer->get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		_requests.resize(0);
		pfd.buffered_requests.resize(0);
	}

	bool buffer_queue::empty(u8 frame_index) const
	{
		return _requests.empty() && _pfd[frame_index].buffered_requests.empty();
	}
}
