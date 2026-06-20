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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/widgets/editor_widget_text_id.hpp"
#include "editor_app.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	void editor_widget_text_id_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_text_id_config_t& config)
	{
		SFG_ASSERT(!config.world.is_null());

		_config = config;

		editor_input_field_config_t input_config = {};
		input_config.type						 = editor_input_field_type_e::text;
		input_config.on_submitted				 = on_submitted;
		input_config.user_data					 = this;
		_input.init(ui, parent, input_config);
		refresh_text();
	}

	void editor_widget_text_id_t::uninit()
	{
		_input.uninit();
		_config = {};
	}

	void editor_widget_text_id_t::refresh_text()
	{
		const u32	   text_id = get_selected();
		const world_t& world   = editor_app_t::get().get_runtime().get_world(_config.world);
		const char*	   text	   = world.get_text(text_id);
		_input.set_text(text != nullptr ? text : "");
	}

	u32 editor_widget_text_id_t::get_selected() const
	{
		return _config.selected != nullptr ? _config.selected(_config.user_data) : ECS_INVALID_INDEX;
	}

	void editor_widget_text_id_t::on_submitted(const char* text, f32, void* user_data)
	{
		editor_widget_text_id_t& widget = *static_cast<editor_widget_text_id_t*>(user_data);
		if (widget._config.submitted != nullptr)
			widget._config.submitted(text, widget._config.user_data);
		widget.refresh_text();
	}
}
