/*
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

#include "data/vector.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "world/world_constants.hpp"
#include "platform/window_common.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class world;

	class editor_gizmo_2d
	{
	public:
		void init(vekt::builder* builder);
		void draw(const vector2& root_pos, const vector2& root_size, const vector2& game_render_size);
		bool on_mouse_event(const window_event& ev);

	private:
		struct icon_entry
		{
			world_handle entity = {};
			vector3		 pos	= vector3::zero;
			const char*	 icon	= nullptr;
		};

		vekt::builder*	   _builder		   = nullptr;
		vector2			   _last_root_pos  = vector2::zero;
		vector2			   _last_root_size = vector2::zero;
		vector<icon_entry> _icons		   = {};
	};
}
