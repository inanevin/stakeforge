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

#include "game/game.hpp"
#include "game/gameplay.hpp"
#include "app/app.hpp"
#include "app/package_manager.hpp"
#include "world/world.hpp"
#include "resources/world_raw.hpp"

namespace SFG
{
	void game::init()
	{
		load_level("assets/world/world0.stkworld");
	}

	void game::uninit()
	{
	}
	void game::tick()
	{
	}

	void game::post_world_tick(f32 delta)
	{
		_app.get_gameplay().on_world_tick(_app.get_world(), delta, vector2ui16());
	}
	bool game::on_window_event(const window_event& ev)
	{
		return false;
	}

	void game::resize(const vector2ui16& size)
	{
	}

	void game::load_level(const char* level)
	{
		package& pck = package_manager::get().open_package_world();

		istream world_stream;
		if (!pck.get_stream(level, world_stream))
		{
			SFG_ERR("failed finding from package: {0}", level);
			return;
		}

		world_raw raw = {};
		raw.deserialize(world_stream);
		pck.close();

		_app.get_world().create_from_loader(raw, false);
		raw.destroy();

		_app.get_world().set_playmode(play_mode::full);
		_app.get_gameplay().on_world_begin(_app.get_world());
	}
}
