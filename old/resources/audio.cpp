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

#include "audio.hpp"
#include "audio_raw.hpp"
#include "io/assert.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"
#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	void audio::reflect()
	{
		reflection::get().register_meta(type_id<audio>::value, 0, "stkaud");
	}
	audio::~audio()
	{
		_flags.set(audio::flags::is_created);
	}

	void audio::create_from_loader(const audio_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(audio::flags::is_created));
		_flags.set(audio::flags::is_created);

		resource_manager&  rm  = w.get_resource_manager();
		chunk_allocator32& aux = rm.get_aux();
		_decoder			   = aux.allocate<ma_decoder>(1);

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = aux.allocate_text(raw.name);
#endif

		_flags.set(audio::flags::is_streaming, raw.is_stream);

		if (_flags.is_set(audio::flags::is_streaming))
		{
		}

		_audio_data = aux.allocate<u8>(raw.audio_data.size());
		SFG_MEMCPY(aux.get(_audio_data.head), raw.audio_data.data(), raw.audio_data.size());

		ma_result result;

		char* raw_data = const_cast<char*>(raw.audio_data.data());
		void* data	   = static_cast<void*>(aux.get(_audio_data.head));

		ma_decoder* dec = aux.get<ma_decoder>(_decoder);

		result = ma_decoder_init_memory(data,
										raw.audio_data.size(),
										nullptr, // auto-detect format
										dec);
		if (result != MA_SUCCESS)
		{
			SFG_ERR("Failed to init decoder from memory for {0}", raw.name);
			return;
		}
	}

	void audio::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(audio::flags::is_created))
			return;
		_flags.remove(audio::flags::is_created);

		resource_manager&  rm  = w.get_resource_manager();
		chunk_allocator32& aux = rm.get_aux();

		ma_decoder* dec = aux.get<ma_decoder>(_decoder);
		ma_decoder_uninit(dec);

		aux.free(_decoder);
		_decoder = {};

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			aux.free(_name);
		_name = {};
#endif

		if (_audio_data.size != 0)
			aux.free(_audio_data);
		_audio_data = {};
	}

	ma_decoder* audio::get_decoder(world& w) const
	{
		resource_manager&  rm  = w.get_resource_manager();
		chunk_allocator32& aux = rm.get_aux();
		return aux.get<ma_decoder>(_decoder);
	}
}
