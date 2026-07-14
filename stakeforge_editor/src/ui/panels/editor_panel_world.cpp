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
#include "ui/panels/editor_panel_world.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>

namespace sfg
{
	editor_panel_world_t::editor_panel_world_t()
	{
		set_type(editor_panel_type_e::world);
		refresh_title();
		set_icon(ICON_GLOBE);
	}

	void editor_panel_world_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_world_view.init(ui, _root);
	}

	void editor_panel_world_t::uninit()
	{
		_world_view.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_world_t::set_edit_world(editor_world_handle_t world)
	{
		_world_view.set_edit_world(world);
	}

	void editor_panel_world_t::set_panel_name(const char* name)
	{
		_panel_name = name;
		refresh_title(_panel_name.c_str(), nullptr, _world_dirty);
	}

	void editor_panel_world_t::set_world_dirty(bool dirty)
	{
		if (_world_dirty == dirty)
			return;

		_world_dirty = dirty;
		refresh_title(_panel_name.c_str(), nullptr, _world_dirty);
	}
}
