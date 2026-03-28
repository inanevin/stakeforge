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

#include "font.hpp"
#include "font_raw.hpp"
#include "gui/vekt.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void font::reflect()
	{
		reflection::get().register_meta(type_id<font>::value, 0, "stkfont");
	}
	font::~font()
	{
		SFG_ASSERT(!_flags.is_set(font::flags::created));
	}
	void font::create_from_loader(const font_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(font::flags::created));
		_flags.set(font::flags::created);

		resource_manager&	rm	  = w.get_resource_manager();
		chunk_allocator32&	alloc = rm.get_aux();
		vekt::font_manager& fm	  = w.get_font_manager();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		// ffs man.
		unsigned char* data = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(raw.font_data.data()));
		_font				= fm.load_font(data, static_cast<unsigned int>(raw.font_data.size()), static_cast<unsigned int>(raw.point_size), 32, 128, static_cast<vekt::font_type>(raw.font_type), raw.sdf_padding, raw.sdf_edge, raw.sdf_distance);
	}

	void font::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(font::flags::created))
			return;
		_flags.remove(font::flags::created);

		resource_manager&	rm	  = w.get_resource_manager();
		chunk_allocator32&	alloc = rm.get_aux();
		vekt::font_manager& fm	  = w.get_font_manager();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		fm.unload_font(_font);
	}
}
