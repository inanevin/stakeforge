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

#include "script_api_game.hpp"

#include <sfg/gfx/util/render_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	namespace
	{
		script_api_game_get_render_resolution_fn g_get_render_resolution = nullptr;
		script_api_game_set_render_resolution_fn g_set_render_resolution = nullptr;
		script_api_game_load_world_fn			 g_load_world			 = nullptr;
	}

	void set_script_api_game_callbacks(script_api_game_get_render_resolution_fn get_resolution, script_api_game_set_render_resolution_fn set_resolution, script_api_game_load_world_fn load_world)
	{
		g_get_render_resolution = get_resolution;
		g_set_render_resolution = set_resolution;
		g_load_world			= load_world;
	}

	u8 api_game_get_render_resolution(vec2u16_t* out_resolution)
	{
		SFG_ASSERT(out_resolution != nullptr);

		*out_resolution = vec2u16_t::zero;

		if (g_get_render_resolution == nullptr)
			return 0;

		return g_get_render_resolution(*out_resolution);
	}

	u8 api_game_set_render_resolution(u16 width, u16 height)
	{
		if (g_set_render_resolution == nullptr)
			return 0;

		vec2u16_t resolution{width, height};
		render_util_t::ensure_world_resolution(resolution);
		return g_set_render_resolution(resolution);
	}

	u8 api_game_load_world(sid_t world)
	{
		if (g_load_world == nullptr)
			return 0;

		return g_load_world(world);
	}

	const script_api_game_t& get_script_api_game()
	{
		static const script_api_game_t api{
			.size				   = static_cast<u32>(sizeof(script_api_game_t)),
			.version			   = 1,
			.get_render_resolution = api_game_get_render_resolution,
			.set_render_resolution = api_game_set_render_resolution,
			.load_world			   = api_game_load_world,
		};

		return api;
	}
}
