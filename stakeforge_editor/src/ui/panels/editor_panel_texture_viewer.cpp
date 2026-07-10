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
#include "ui/panels/editor_panel_texture_viewer.hpp"
#include "editor_app.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	editor_panel_texture_viewer_t::editor_panel_texture_viewer_t()
	{
		set_type(editor_panel_type_e::texture_viewer);
		set_title(editor_panel_type_to_string(editor_panel_type_e::texture_viewer));
		set_icon(ICON_EYE);
	}

	void editor_panel_texture_viewer_t::serialize(nlohmann::json& j) const
	{
		j				  = nlohmann::json::object();
		j["texture_guid"] = _texture_guid;
		j["asset_name"]	  = _asset_name;
	}

	void editor_panel_texture_viewer_t::deserialize(const nlohmann::json& j)
	{
		_texture_guid = j.value<sid_t>("texture_guid", 0);
		_asset_name	  = j.value<string_t>("asset_name", {});
		refresh_title();
	}

	void editor_panel_texture_viewer_t::set_texture(sid_t texture_guid, const char* asset_name)
	{
		SFG_ASSERT(asset_name != nullptr);
		_texture_guid = texture_guid;
		_asset_name	  = asset_name;
		refresh_title();
	}

	void editor_panel_texture_viewer_t::refresh_title()
	{
		_title_text = editor_panel_type_to_string(editor_panel_type_e::texture_viewer);
		if (!_asset_name.empty())
		{
			_title_text = "T: ";
			_title_text += _asset_name;
		}
		set_title(_title_text.c_str());
		if (_ui != nullptr)
			editor_app_t::get().refresh_panel_title(this);
	}
}
