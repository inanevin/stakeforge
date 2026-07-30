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

#include "script_api_screen.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	u8 api_screen_world_to_screen(const world_t* world, const vec3f_t* world_position, vec2f_t* out_screen_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(world_position != nullptr);
		SFG_ASSERT(out_screen_position != nullptr);

		return world->get_screen().world_to_screen(*world_position, *out_screen_position) ? 1 : 0;
	}

	u8 api_screen_screen_to_world(const world_t* world, const vec2f_t* screen_position, f32 ndc_depth, vec3f_t* out_world_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(screen_position != nullptr);
		SFG_ASSERT(out_world_position != nullptr);

		return world->get_screen().screen_to_world(*screen_position, ndc_depth, *out_world_position) ? 1 : 0;
	}

	const script_api_screen_t& get_script_api_screen()
	{
		static const script_api_screen_t api{
			.size			 = static_cast<u32>(sizeof(script_api_screen_t)),
			.version		 = 1,
			.world_to_screen = api_screen_world_to_screen,
			.screen_to_world = api_screen_screen_to_world,
		};

		return api;
	}
}
