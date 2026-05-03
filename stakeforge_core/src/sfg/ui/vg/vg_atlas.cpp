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

#include "vg_atlas.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>

namespace sfg::ui
{
	vg_atlas_t::~vg_atlas_t()
	{
		SFG_ASSERT(_data == nullptr);
	}

	void vg_atlas_t::init(u32 width, u32 height, bool is_lcd)
	{
		SFG_ASSERT(width > 0 && height > 0);
		_width	   = width;
		_height	   = height;
		_is_lcd	   = is_lcd;
		_data_size = width * height * (is_lcd ? 3u : 1u);
		_data	   = static_cast<u8*>(SFG_MALLOC(_data_size));
		SFG_MEMTRACE_ALLOC(_data, _data_size);
		std::memset(_data, 0, _data_size);
		_dirty = true;
	}

	void vg_atlas_t::uninit()
	{
		if (_data)
		{
			SFG_MEMTRACE_DEALLOC(_data);
			SFG_FREE(_data);
		}
		_data	   = nullptr;
		_data_size = 0;
		_dirty	   = false;
	}
}
