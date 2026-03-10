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
#include "data/vector.hpp"
#include "common/size_definitions.hpp"
#include "game/game_max_defines.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/barrier_description.hpp"
#include <functional>

namespace SFG
{
	struct texture_buffer;
	struct barrier;

	class texture_queue
	{
	private:
		struct texture_request
		{
			static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
			gfx_id											texture		 = 0;
			gfx_id											intermediate = 0;
			u64												added_frame	 = 0;
			u8												cleared		 = 0;
			u8												use_free	 = 0;
			bool											persistent	 = false;
			resource_state									to_state	 = resource_state::resource_state_ps_resource;
		};

	public:
		void init();
		void uninit();

		void add_request(const static_vector<texture_buffer, MAX_TEXTURE_MIPS>& buffers, gfx_id texture, gfx_id intermediate, u8 use_free, resource_state state, bool persistent);
		void flush_all(gfx_id cmd, vector<barrier>& out_barriers);
		bool empty() const;

	private:
		vector<texture_request> _requests = {};
	};
}
