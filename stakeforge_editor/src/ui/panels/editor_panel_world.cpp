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
#include "editor_app.hpp"
#include "editor_world_controller.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

namespace sfg
{
	editor_panel_world_t::editor_panel_world_t()
	{
		set_type(editor_panel_type_e::world);
		_title_text = editor_panel_type_to_string(editor_panel_type_e::world);
		set_title(_title_text.c_str());
		set_icon(ICON_GLOBE);
	}

	void editor_panel_world_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_world_view.init(ui, _root);

		editor_world_controller_t&	controller = editor_world_controller_t::get();
		const editor_world_handle_t main_world = controller.get_main_world();
		_world_view.set_edit_context(controller.get_main_world());
		set_panel_name(main_world.is_null() ? "" : controller.get_main_world_name());
		set_world_dirty(controller.is_main_world_dirty());
	}

	void editor_panel_world_t::uninit()
	{
		_world_view.uninit();
		editor_panel_t::uninit();
	}

	void editor_panel_world_t::set_edit_context(editor_world_handle_t context)
	{
		_world_view.set_edit_context(context);
	}

	void editor_panel_world_t::set_panel_name(const char* name)
	{
		SFG_ASSERT(name != nullptr);

		_panel_name = name;
		refresh_title();
	}

	void editor_panel_world_t::set_world_dirty(bool dirty)
	{
		if (_world_dirty == dirty)
			return;

		_world_dirty = dirty;
		refresh_title();
	}

	vec4f_t editor_panel_world_t::get_world_view_bounds() const
	{
		return _world_view.get_world_view_bounds();
	}

	void editor_panel_world_t::refresh_title()
	{
		const sid_t old_identifier = TO_SID(get_title());
		_title_text				   = editor_panel_type_to_string(editor_panel_type_e::world);
		if (!_panel_name.empty())
		{
			_title_text += ": ";
			_title_text += _panel_name;
		}
		if (_world_dirty)
			_title_text += "*";
		set_title(_title_text.c_str());
		if (_ui != nullptr)
			editor_app_t::get().refresh_panel_title(this, old_identifier);
	}
}
