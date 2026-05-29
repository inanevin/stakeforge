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

#include "world_render_context.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	world_render_context_t::world_render_context_t(world_render_context_t&& other) noexcept
	{
		*this = static_cast<world_render_context_t&&>(other);
	}

	world_render_context_t& world_render_context_t::operator=(world_render_context_t&& other) noexcept
	{
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i]		  = other._pfd[i];
			other._pfd[i] = {};
		}
		_size		= other._size;
		other._size = vec2u16_t::zero;
		return *this;
	}

	void world_render_context_t::init(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].command_buffer.is_null());
			SFG_ASSERT(_pfd[i].world_texture.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());

			_pfd[i].command_buffer = gfx_backend::get().create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "world_gfx",
			});
		}
		create_texture(size);
	}

	void world_render_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_texture();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].command_buffer.is_null());
			gfx_backend::get().destroy_command_buffer(_pfd[i].command_buffer);
			_pfd[i].command_buffer = {};
		}
	}

	void world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		destroy_texture();
		create_texture(size);
	}

	gfx_command_buffer_handle world_render_context_t::get_command_buffer(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].command_buffer;
	}

	gfx_texture_handle world_render_context_t::get_world_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].world_texture;
	}

	gfx_texture_handle world_render_context_t::get_depth_texture(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].depth_texture;
	}

	gpu_index_t world_render_context_t::get_world_texture_index(u8 frame_index) const
	{
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);
		return _pfd[frame_index].world_texture_index;
	}

	vec2u16_t world_render_context_t::get_size() const
	{
		return _size;
	}

	void world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t color_desc = {};
		color_desc.texture_format = format_e::r8g8b8a8_unorm;
		color_desc.size			  = size;
		color_desc.flags		  = texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		color_desc.view_count	  = 2;
		color_desc.views[0]		  = {.type = view_type::sampled};
		color_desc.views[1]		  = {.type = view_type::render_target};
		color_desc.set_name("world_texture");

		texture_desc_t depth_desc		= {};
		depth_desc.texture_format		= format_e::d32_sfloat;
		depth_desc.depth_stencil_format = format_e::d32_sfloat;
		depth_desc.size					= size;
		depth_desc.flags				= texture_flags::tf_depth_texture | texture_flags::tf_is_2d;
		depth_desc.view_count			= 1;
		depth_desc.views[0]				= {.type = view_type::depth_stencil};
		depth_desc.clear_values[0]		= 0.0f;
		depth_desc.set_name("world_depth");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].world_texture.is_null());
			SFG_ASSERT(_pfd[i].depth_texture.is_null());

			_pfd[i].world_texture		= backend.create_texture(color_desc);
			_pfd[i].depth_texture		= backend.create_texture(depth_desc);
			_pfd[i].world_texture_index = backend.get_texture_gpu_index(_pfd[i].world_texture, 0);
		}
		_size = size;
	}

	void world_render_context_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].world_texture.is_null());
			SFG_ASSERT(!_pfd[i].depth_texture.is_null());

			backend.destroy_texture(_pfd[i].world_texture);
			backend.destroy_texture(_pfd[i].depth_texture);
			_pfd[i].world_texture		= {};
			_pfd[i].depth_texture		= {};
			_pfd[i].world_texture_index = NULL_GPU_INDEX;
		}
		_size = vec2u16_t::zero;
	}
}
