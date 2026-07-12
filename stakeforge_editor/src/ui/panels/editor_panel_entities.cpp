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
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

namespace sfg
{
	editor_panel_entities_t::editor_panel_entities_t()
	{
		set_type(editor_panel_type_e::entities);
		set_title(editor_panel_type_to_string(editor_panel_type_e::entities));
		set_icon(ICON_BOXES);
	}

	void editor_panel_entities_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_outliner.init(ui, _root);
		_outliner.refresh_entities();
	}

	void editor_panel_entities_t::uninit()
	{
		_outliner.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_entities_t::refresh_entities()
	{
		_outliner.refresh_entities();
	}

	void editor_panel_entities_t::refresh_entity_name(entity_id_t entity)
	{
		_outliner.refresh_entity_name(entity);
	}

	void editor_panel_entities_t::set_edit_world(editor_world_handle_t world)
	{
		_outliner.set_edit_world(world);
	}

	void editor_panel_entities_t::show_entity(entity_guid_t guid)
	{
		_outliner.show_entity(guid);
	}
}
