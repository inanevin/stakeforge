/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include <sfg/common/type_id.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
	struct editor_project_cook_options_t
	{
		vector_t<resource_handle_t> worlds			= {};
		vector_t<resource_handle_t> extra_resources = {};
		resource_handle_t			main_world		= NULL_RESOURCE_HANDLE;
		vec2u16_t					resolution		= {1920, 1080};
		bool						is_borderless	= true;
		bool						is_fullscreen	= false;
	};

	SFG_DEFINE_TYPE_ID(editor_project_cook_options_t);

	struct editor_project_cook_options_reflection_t
	{
		editor_project_cook_options_reflection_t();
	};

	inline editor_project_cook_options_reflection_t g_reflect_editor_project_cook_options;
}
