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

#define NOMINMAX
#include <sfg/common/size_definitions.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
#define BACK_BUFFER_COUNT 3
#define FRAME_LATENCY	  2
	inline constexpr u8 TEXTURE_MAX_VIEWS = 12;

	// 0 discrete, 1 integratd
#define GPU_DEVICE 0

	typedef unsigned short gfx_id_t;
	typedef u32			   primitive_index;
	typedef unsigned int   gpu_index_t;

	struct gfx_handle_tag
	{
	};

	typedef pool_handle_t<gfx_id_t, gfx_handle_tag> gfx_handle_t;

#define NULL_GFX_ID	   (unsigned short)0xFFFF
#define NULL_GPU_INDEX (unsigned int)0xFFFFFFFF
}
