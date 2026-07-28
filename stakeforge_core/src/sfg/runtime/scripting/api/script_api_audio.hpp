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

#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	u8	 api_audio_play(world_t* world, entity_id_t entity);
	u8	 api_audio_pause(world_t* world, entity_id_t entity);
	u8	 api_audio_stop(world_t* world, entity_id_t entity);
	void api_audio_pause_all(world_t* world);
	void api_audio_resume_all(world_t* world);

	struct script_api_audio_t
	{
		u32 size										= 0;
		u32 version										= 0;
		u8 (*play)(world_t* world, entity_id_t entity)	= nullptr;
		u8 (*pause)(world_t* world, entity_id_t entity) = nullptr;
		u8 (*stop)(world_t* world, entity_id_t entity)	= nullptr;
		void (*pause_all)(world_t* world)				= nullptr;
		void (*resume_all)(world_t* world)				= nullptr;
	};

	const script_api_audio_t& get_script_api_audio();
}
