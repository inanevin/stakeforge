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

#include "world/editor_world_render_context.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	void editor_world_render_context_t::init(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		create_texture(size);
	}

	void editor_world_render_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_texture();
	}

	void editor_world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		destroy_texture();
		create_texture(size);
	}

	void editor_world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t desc = {};
		desc.texture_format = format_e::r8g8b8a8_srgb;
		desc.size			= size;
		desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		desc.view_count		= 2;
		desc.views[0]		= {.type = view_type::render_target};
		desc.views[1]		= {.type = view_type::sampled};
		desc.set_name("editor_world_texture");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].world_texture.is_null());

			_pfd[i].world_texture		= backend.create_texture(desc);
			_pfd[i].world_texture_index = backend.get_texture_gpu_index(_pfd[i].world_texture, 1);
		}
		_size = size;
	}

	void editor_world_render_context_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].world_texture.is_null());

			backend.destroy_texture(_pfd[i].world_texture);
			_pfd[i].world_texture		= {};
			_pfd[i].world_texture_index = NULL_GPU_INDEX;
		}
		_size = vec2u16_t::zero;
	}
}
