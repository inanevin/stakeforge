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

#include "render_event_stream.hpp"
#include "data/istream.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{
	void render_event_stream::init()
	{
		_main_thread_data.create(RENDER_STREAM_BATCH_SIZE);

		for (u32 i = 0; i < RENDER_STREAM_MAX_BATCHES; i++)
		{
			_stream_data[i].data = new u8[RENDER_STREAM_BATCH_SIZE];
		}

		for (u32 i = 0; i < TRANSFORM_BUFFERS; i++)
		{
			_transform_data[i].entities = new proxy_entity_transform_data[MAX_ENTITIES];
			_transform_data[i].dirty_indices.reserve(MAX_ENTITIES);
			_transform_data[i].dirty_flags = new u8[MAX_ENTITIES];
			SFG_MEMSET(_transform_data[i].dirty_flags, 0, sizeof(u8) * MAX_ENTITIES);
			_transform_data[i].peak_size = 0;
		}
	}

	void render_event_stream::uninit()
	{
		_main_thread_data.destroy();

		for (u32 i = 0; i < RENDER_STREAM_MAX_BATCHES; i++)
		{
			delete[] _stream_data[i].data;

			_stream_data[i].data = nullptr;
		}

		for (u32 i = 0; i < TRANSFORM_BUFFERS; i++)
		{
			delete[] _transform_data[i].entities;
			delete[] _transform_data[i].dirty_flags;
			_transform_data[i].entities	   = nullptr;
			_transform_data[i].dirty_flags = nullptr;
			_transform_data[i].dirty_indices.resize(0);
		}
	}

	void render_event_stream::publish()
	{
		ZoneScoped;

		const size_t sz = _main_thread_data.get_size();
		if (sz != 0)
		{
			SFG_ASSERT(sz < RENDER_STREAM_BATCH_SIZE);

			const i8 last_rendered = _events_rendered.load(std::memory_order_acquire);
			_events_write		   = (last_rendered + 1) % RENDER_STREAM_MAX_BATCHES;

			buffered_data& buf	   = _stream_data[_events_write];
			const size_t   data_sz = _main_thread_data.get_size();

			if (buf.size + data_sz >= RENDER_STREAM_BATCH_SIZE)
			{
				SFG_FATAL("writing too much data to event_stream data! grow RENDER_STREAM_BATCH_SIZE!");
				return;
			}

			SFG_MEMCPY(buf.data + buf.size, _main_thread_data.get_raw(), data_sz);
			buf.size += data_sz;

			_main_thread_data.shrink(0);
			_events_latest.store(_events_write, std::memory_order_release);
		}

		const u8 published = _transform_write;
		_transform_latest.store((i8)published, std::memory_order_release);

		// pick next write buffer that is NOT currently in use by render
		const i8 in_use = _transform_in_use.load(std::memory_order_acquire);

		u8 next = (published + 1) % TRANSFORM_BUFFERS;
		if ((i8)next == in_use)
			next = (next + 1) % TRANSFORM_BUFFERS;

		_transform_write = next;

		reset_xform_buffer(_transform_data[_transform_write]);
	}

	void render_event_stream::add_entity_transform_event(world_id index, const matrix4x3& model, const quat& rot)
	{
		proxy_entity_data& ped = _transform_data[_transform_write];

		if (index + 1 > ped.peak_size)
			ped.peak_size = index + 1;

		proxy_entity_transform_data& e = ped.entities[index];
		e.rot						   = rot;
		e.model						   = model;

		u8& dirty = ped.dirty_flags[index];
		if (dirty == 0)
		{
			ped.dirty_indices.push_back(index);
			dirty = 1;
		}
	}

	void render_event_stream::read(render_proxy_entity* out_entities, u32& out_size, istream& stream)
	{
		ZoneScoped;

		const i8 transform_idx = _transform_latest.load(std::memory_order_acquire);
		if (transform_idx >= 0)
		{
			_transform_in_use.store(transform_idx, std::memory_order_release);

			proxy_entity_data& ped = _transform_data[(u8)transform_idx];

			for (world_id d : ped.dirty_indices)
			{
				proxy_entity_transform_data& e = ped.entities[d];

				render_proxy_entity& write = out_entities[d];
				write.rotation			   = e.rot;
				write.model				   = e.model;
				write.normal			   = e.model.to_linear3x3().inversed().transposed();
			}

			out_size = ped.peak_size;
			reset_xform_buffer(ped);
			_transform_in_use.store(-1, std::memory_order_release);
		}

		const i8 idx = _events_latest.load(std::memory_order_acquire);
		if (idx < 0)
			return;

		buffered_data& buf = _stream_data[idx];
		stream.open(buf.data, buf.size);
		buf.size = 0;
		_events_rendered.store(idx, std::memory_order_release);
	}

}