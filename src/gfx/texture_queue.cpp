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

#include "texture_queue.hpp"
#include "data/vector_util.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "common/system_info.hpp"
#include "memory/memory.hpp"

namespace SFG
{

	void texture_queue::init()
	{
		_requests.reserve(256);
	}

	void texture_queue::uninit()
	{
	}

	void texture_queue::add_request(const static_vector<texture_buffer, MAX_TEXTURE_MIPS>& buffers, gfx_id texture, gfx_id intermediate, u8 use_free, resource_state state, bool persistent)
	{
		for (texture_request& req : _requests)
		{
			if (req.texture == texture)
			{
				for (texture_buffer& b : req.buffers)
				{
					if (req.use_free)
						SFG_FREE(b.pixels);
					else
						delete[] b.pixels;
				}

				req.buffers.clear();
				req.buffers = buffers;
				return;
			}
		}

		const texture_request req = {
			.buffers	  = buffers,
			.texture	  = texture,
			.intermediate = intermediate,
			.added_frame  = frame_info::get_render_frame(),
			.use_free	  = use_free,
			.persistent	  = persistent,
			.to_state	  = state,
		};

		_requests.push_back(req);
	}

	void texture_queue::flush_all(gfx_id cmd_list, vector<barrier>& out_barriers)
	{
		gfx_backend* backend = gfx_backend::get();

		for (texture_request& buf : _requests)
		{
			backend->cmd_copy_buffer_to_texture(cmd_list,
												{
													.textures			 = buf.buffers.data(),
													.destination_texture = buf.texture,
													.intermediate_buffer = buf.intermediate,
													.mip_levels			 = static_cast<u8>(buf.buffers.size()),
													.destination_slice	 = 0,

												});

			out_barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = buf.to_state,
				.resource	 = buf.texture,
				.flags		 = barrier_flags::baf_is_texture,
			});

			for (const texture_buffer& b : buf.buffers)
			{
				if (buf.persistent)
					continue;

				if (buf.use_free)
					SFG_FREE(b.pixels);
				else
					delete[] b.pixels;
			}
		}

		_requests.resize(0);
	}

	bool texture_queue::empty() const
	{
		return _requests.empty();
	}

}
