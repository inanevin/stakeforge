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

namespace sfg
{
	struct vec2u16_t;

	typedef u8 (*script_api_game_get_render_resolution_fn)(vec2u16_t& out_resolution);
	typedef u8 (*script_api_game_set_render_resolution_fn)(const vec2u16_t& resolution);
	typedef u8 (*script_api_game_load_world_fn)(sid_t world_name_hash);
	typedef u8 (*script_api_game_restart_world_fn)();
	typedef void (*script_api_game_quit_fn)();

	void set_script_api_game_callbacks(
		script_api_game_get_render_resolution_fn get_resolution, script_api_game_set_render_resolution_fn set_resolution, script_api_game_load_world_fn load_world, script_api_game_restart_world_fn restart_world, script_api_game_quit_fn quit);
	u8	 api_game_get_render_resolution(vec2u16_t* out_resolution);
	u8	 api_game_set_render_resolution(u16 width, u16 height);
	u8	 api_game_load_world(sid_t world_name_hash);
	u8	 api_game_restart_world();
	void api_game_quit();

	struct script_api_game_t
	{
		u32										  size					= 0;
		u32										  version				= 0;
		decltype(&api_game_get_render_resolution) get_render_resolution = nullptr;
		decltype(&api_game_set_render_resolution) set_render_resolution = nullptr;
		decltype(&api_game_load_world)			  load_world			= nullptr;
		decltype(&api_game_restart_world)		  restart_world			= nullptr;
		decltype(&api_game_quit)				  quit					= nullptr;
	};

	const script_api_game_t& get_script_api_game();
}
