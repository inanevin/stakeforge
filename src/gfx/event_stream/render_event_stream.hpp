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

#include "common/size_definitions.hpp"
#include "data/ostream.hpp"
#include "gfx/event_stream/render_events.hpp"
#include "game/game_max_defines.hpp"
#include "io/log.hpp"
#include "data/atomic.hpp"
#include "gfx/proxy/render_proxy_entity.hpp"

namespace SFG
{
	class istream;
	struct render_proxy_entity;

	class render_event_stream
	{
	public:
		static constexpr u8 TRANSFORM_BUFFERS = 3;

		struct buffered_data
		{
			u8*	   data = nullptr;
			size_t size = 0;
		};

		struct proxy_entity_transform_data
		{
			matrix4x3 model = matrix4x3::identity;
			quat	  rot	= quat::identity;
		};
		struct proxy_entity_data
		{
			proxy_entity_transform_data* entities	   = nullptr;
			vector<world_id>			 dirty_indices = {};
			u8*							 dirty_flags   = nullptr;
			u32							 peak_size	   = 0;
		};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void publish();
		void add_entity_transform_event(world_id index, const matrix4x3& model, const quat& rot);
		void read(render_proxy_entity* out_entities, u32& out_size, istream& stream);

		// -----------------------------------------------------------------------------
		// event api
		// -----------------------------------------------------------------------------

		template <typename T> inline void add_event(const render_event_header& header, const T& ev)
		{
			header.serialize(_main_thread_data);
			ev.serialize(_main_thread_data);
			SFG_ASSERT(_main_thread_data.get_size() < RENDER_STREAM_BATCH_SIZE);
		}

		inline void add_event(const render_event_header& header)
		{
			header.serialize(_main_thread_data);
		}

	private:
		inline void reset_xform_buffer(render_event_stream::proxy_entity_data& ped)
		{
			for (world_id idx : ped.dirty_indices)
				ped.dirty_flags[idx] = 0;
			ped.dirty_indices.resize(0);
			ped.peak_size = 0;
		}

	private:
		buffered_data	  _stream_data[RENDER_STREAM_MAX_BATCHES];
		proxy_entity_data _transform_data[TRANSFORM_BUFFERS];
		atomic<i8>		  _events_latest	= {-1};
		atomic<i8>		  _events_rendered	= {-1};
		u8				  _events_write		= 0;
		ostream			  _main_thread_data = {};

		std::atomic<i8> _transform_latest = {-1}; // last published transform buffer
		std::atomic<i8> _transform_in_use = {-1}; // render thread sets while reading
		u8				_transform_write  = 0;	  // game thread writes here
	};
}