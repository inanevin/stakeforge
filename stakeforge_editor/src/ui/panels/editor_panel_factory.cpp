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
#include "ui/panels/editor_panel_factory.hpp"
#include "ui/panels/editor_panel_animation.hpp"
#include "ui/panels/editor_panel_assets.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_panel_inspector.hpp"
#include "ui/panels/editor_panel_log.hpp"
#include "ui/panels/editor_panel_profiling.hpp"
#include "ui/panels/editor_panel_widget_test.hpp"
#include "ui/panels/editor_panel_world.hpp"

namespace sfg
{
	editor_panel_t* editor_panel_factory_t::create_panel(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return new editor_panel_entities_t();
		case editor_panel_type_e::assets:
			return new editor_panel_assets_t();
		case editor_panel_type_e::log:
			return new editor_panel_log_t();
		case editor_panel_type_e::world:
			return new editor_panel_world_t();
		case editor_panel_type_e::inspector:
			return new editor_panel_inspector_t();
		case editor_panel_type_e::animation:
			return new editor_panel_animation_t();
		case editor_panel_type_e::profiling:
			return new editor_panel_profiling_t();
		case editor_panel_type_e::widget_test:
			return new editor_panel_widget_test_t();
		default:
			return nullptr;
		}
	}

	void editor_panel_factory_t::delete_panel(editor_panel_t* panel)
	{
		delete panel;
	}
}
