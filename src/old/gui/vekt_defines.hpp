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

namespace vekt
{
	typedef unsigned int   id;
	typedef unsigned short index;
#define NULL_WIDGET_ID 0xffffffffui32

	class builder;
	struct mouse_event;
	struct mouse_wheel_event;
	struct key_event;
	enum class input_event_phase;
	enum class input_event_result;
	struct VEKT_VEC2;

	typedef void (*widget_func)(builder* b, id widget);
	typedef void (*focus_gain_func)(builder* b, id widget, bool from_nav);
	typedef input_event_result (*mouse_func)(builder* b, id widget, const mouse_event& ev, input_event_phase phase);
	typedef void (*drag_func)(builder* b, id widget, f32 mouse_x, f32 mouse_y, f32 delta_x, f32 delta_y, unsigned int button);
	typedef input_event_result (*key_func)(builder* b, id widget, const key_event& ev);
	typedef input_event_result (*wheel_func)(builder* b, id widget, const mouse_wheel_event& ev);

}
