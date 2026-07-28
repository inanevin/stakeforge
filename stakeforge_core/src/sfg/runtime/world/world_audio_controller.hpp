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

#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	class world_audio_controller_t final
	{
	public:
		world_audio_controller_t()											 = default;
		~world_audio_controller_t()											 = default;
		world_audio_controller_t(const world_audio_controller_t&)			 = delete;
		world_audio_controller_t& operator=(const world_audio_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world);
		void uninit();
		void begin_play();
		void end_play();
		void destroy_entity(entity_id_t entity);
		void tick(f32 delta_time);

		// -----------------------------------------------------------------------------
		// playback
		// -----------------------------------------------------------------------------

		bool play(entity_id_t entity);
		void pause(entity_id_t entity);
		void stop(entity_id_t entity);
		void pause_all();
		void resume_all();

	private:
		bool create_voice(entity_id_t entity, bool start);
		void destroy_voice(entity_id_t entity);
		void sync_sources(f32 delta_time);
		void sync_listener(f32 delta_time);

	private:
		world_t* _world		 = nullptr;
		bool	 _is_playing = false;
		bool	 _is_paused	 = false;
	};
}
