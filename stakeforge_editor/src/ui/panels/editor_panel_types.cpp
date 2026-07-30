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
#include "ui/panels/editor_panel_types.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	const char* editor_panel_type_to_string(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return "Entities";
		case editor_panel_type_e::assets:
			return "Assets";
		case editor_panel_type_e::log:
			return "Log";
		case editor_panel_type_e::world:
			return "World";
		case editor_panel_type_e::inspector:
			return "Inspector";
		case editor_panel_type_e::animation:
			return "Animation";
		case editor_panel_type_e::resources:
			return "Resources";
		case editor_panel_type_e::project_settings:
			return "Project Settings";
		case editor_panel_type_e::mesh_viewer:
			return "Mesh Viewer";
		case editor_panel_type_e::skeleton_viewer:
			return "Skeleton Viewer";
		case editor_panel_type_e::ragdoll_viewer:
			return "Ragdoll Viewer";
		case editor_panel_type_e::animation_graph:
			return "Animation Graph";
		default:
			return "";
		}
	}

	editor_panel_type_e editor_panel_type_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
		{
			const editor_panel_type_e type = static_cast<editor_panel_type_e>(i);
			if (TO_SID(editor_panel_type_to_string(type)) == id)
				return type;
		}
		return editor_panel_type_e::max;
	}
}
