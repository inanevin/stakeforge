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

#include "editor/gui/editor_gui_builder.hpp"

namespace vekt
{
	class builder;
};

namespace SFG
{
	struct vector2ui16;

	class editor_panel_stats
	{
	public:
		void init(vekt::builder* builder);
		void uninit();
		void draw(const vector2ui16& window_size);
		void set_visible(bool is_visible);

		inline vekt::id get_root() const
		{
			return _gui_builder.get_root();
		}

	private:
		vekt::builder* _builder		= nullptr;
		gui_builder	   _gui_builder = {};

		vekt::id _stats_area		= NULL_WIDGET_ID;
		vekt::id _wv_game_res		= NULL_WIDGET_ID;
		vekt::id _wv_window_res		= NULL_WIDGET_ID;
		vekt::id _wv_fps			= NULL_WIDGET_ID;
		vekt::id _wv_main			= NULL_WIDGET_ID;
		vekt::id _wv_render			= NULL_WIDGET_ID;
		vekt::id _wv_ram			= NULL_WIDGET_ID;
		vekt::id _wv_vram			= NULL_WIDGET_ID;
		vekt::id _wv_vram_txt		= NULL_WIDGET_ID;
		vekt::id _wv_vram_res		= NULL_WIDGET_ID;
		vekt::id _wv_draw_calls		= NULL_WIDGET_ID;
		vekt::id _wv_loaded_project = NULL_WIDGET_ID;
		vekt::id _wv_loaded_level	= NULL_WIDGET_ID;
	};
}
