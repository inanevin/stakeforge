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

#include "texture.hpp"
#include "texture_raw.hpp"
#include "io/assert.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void texture::reflect()
	{
		reflection::get().register_meta(type_id<texture>::value, 0, "stktexture");
	}
	texture::~texture()
	{
		SFG_ASSERT(!_flags.is_set(texture::flags::created));
	}

	void texture::create_from_loader(const texture_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(texture::flags::created));
		_flags.set(texture::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

		_texture_format = raw.texture_format;
		SFG_ASSERT(!raw.buffers.empty());

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		gfx_backend* backend	= gfx_backend::get();
		u32			 total_size = 0;
		for (const texture_buffer& buf : raw.buffers)
			total_size += backend->get_texture_size(buf.size.x, buf.size.y, buf.bpp);

		render_event_texture stg = {};
		stg.buffers				 = raw.buffers;
		stg.format				 = _texture_format;
		stg.size				 = raw.buffers[0].size;
		stg.intermediate_size	 = backend->align_texture_size(total_size);
		stg.buffers_persistent	 = raw.buffers_persistent;

#ifndef SFG_STRIP_DEBUG_NAMES
		stg.name = raw.name;
#endif

		stream.add_event(
			{
				.index		= static_cast<u32>(handle.index),
				.event_type = render_event_type::create_texture,
			},
			stg);
	}

	void texture::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(texture::flags::created))
			return;

		_flags.remove(texture::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		stream.add_event({
			.index		= static_cast<u32>(handle.index),
			.event_type = render_event_type::destroy_texture,
		});
	}

}
