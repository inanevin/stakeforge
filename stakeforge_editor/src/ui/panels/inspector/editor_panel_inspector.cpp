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
#include "ui/panels/inspector/editor_panel_inspector.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

namespace sfg
{
	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
		set_icon(ICON_GLASSES);
	}

	void editor_panel_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_inspector.init(ui,
						_root,
						{
							.allow_prefab_blocks = true,
						});
		_inspector.refresh_from_selection();
	}

	void editor_panel_inspector_t::uninit()
	{
		_inspector.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_inspector_t::set_display_none()
	{
		_inspector.set_display_none();
	}

	void editor_panel_inspector_t::set_display_entity(entity_id_t entity)
	{
		_inspector.set_display_entity(entity);
	}

	void editor_panel_inspector_t::set_display_entity(span_t<const entity_id_t> entities)
	{
		_inspector.set_display_entity(entities);
	}

	void editor_panel_inspector_t::refresh_display()
	{
		_inspector.refresh_display();
	}

	void editor_panel_inspector_t::refresh_from_selection()
	{
		_inspector.refresh_from_selection();
	}

	void editor_panel_inspector_t::refresh_component_reflection(sid_t component_type)
	{
		_inspector.refresh_component_reflection(component_type);
	}

	void editor_panel_inspector_t::set_edit_world(editor_world_handle_t world)
	{
		_inspector.set_edit_world(world);
	}
}
